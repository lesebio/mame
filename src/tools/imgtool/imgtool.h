// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    imgtool.h

    Main headers for Imgtool core

***************************************************************************/

#ifndef IMGTOOL_H
#define IMGTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "corestr.h"
#include "formats/flopimg.h"
#include "opresolv.h"
#include "library.h"
#include "filter.h"

/* ----------------------------------------------------------------------- */

#define EOLN_CR     "\x0d"
#define EOLN_LF     "\x0a"
#define EOLN_CRLF   "\x0d\x0a"

#define FILENAME_BOOTBLOCK  ((const char *) 1)

enum
{
	OSD_FOPEN_READ,
	OSD_FOPEN_WRITE,
	OSD_FOPEN_RW,
	OSD_FOPEN_RW_CREATE
};

/* ---------------------------------------------------------------------------
 * Image calls
 *
 * These are the calls that front ends should use for manipulating images. You
 * should never call the module functions directly because they may not be
 * implemented (i.e. - the function pointers are NULL). The img_* functions are
 * aware of these issues and will make the appropriate checks as well as
 * marking up return codes with the source.  In addition, some of the img_*
 * calls are high level calls that simply image manipulation
 *
 * Calls that return 'int' that are not explictly noted otherwise return
 * imgtool error codes
 * ---------------------------------------------------------------------------
 */

struct imgtool_module_features
{
	unsigned int supports_create : 1;
	unsigned int supports_open : 1;
	unsigned int supports_readsector : 1;
	unsigned int supports_writesector : 1;
	unsigned int is_read_only : 1;
};

struct imgtool_partition_features
{
	unsigned int supports_reading : 1;
	unsigned int supports_writing : 1;
	unsigned int supports_deletefile : 1;
	unsigned int supports_directories : 1;
	unsigned int supports_freespace : 1;
	unsigned int supports_createdir : 1;
	unsigned int supports_deletedir : 1;
	unsigned int supports_creation_time : 1;
	unsigned int supports_lastmodified_time : 1;
	unsigned int supports_forks : 1;
	unsigned int supports_geticoninfo : 1;
	unsigned int is_read_only : 1;
};

/* ----- initialization and basics ----- */
void imgtool_init(bool omit_untested, void (*warning)(const char *message));
void imgtool_exit(void);
const imgtool_module *imgtool_find_module(const std::string &modulename);
const imgtool::library::modulelist &imgtool_get_modules();
imgtool_module_features imgtool_get_module_features(const imgtool_module *module);
void imgtool_warn(const char *format, ...) ATTR_PRINTF(1,2);

// ----- image management -----
namespace imgtool
{
	class image
	{
	public:
		typedef std::unique_ptr<image> ptr;

		image(const imgtool_module &module, object_pool *pool, void *extra_bytes);
		~image();
		
		static imgtoolerr_t identify_file(const char *filename, imgtool_module **modules, size_t count);
		static imgtoolerr_t open(const imgtool_module *module, const char *filename, int read_or_write, ptr &outimg);
		static imgtoolerr_t open(const std::string &modulename, const char *filename, int read_or_write, ptr &outimg);
		static imgtoolerr_t create(const imgtool_module *module, const char *fname, util::option_resolution *opts, ptr &image);
		static imgtoolerr_t create(const std::string &modulename, const char *fname, util::option_resolution *opts, ptr &image);
		static imgtoolerr_t create(const imgtool_module *module, const char *fname, util::option_resolution *opts);
		static imgtoolerr_t create(const std::string &modulename, const char *fname, util::option_resolution *opts);
		static UINT64 rand();

		imgtoolerr_t info(char *string, size_t len);
		imgtoolerr_t get_geometry(UINT32 *tracks, UINT32 *heads, UINT32 *sectors);
		imgtoolerr_t read_sector(UINT32 track, UINT32 head, UINT32 sector, std::vector<UINT8> &buffer);
		imgtoolerr_t write_sector(UINT32 track, UINT32 head, UINT32 sector, const void *buffer, size_t len);
		imgtoolerr_t get_block_size(UINT32 &length);
		imgtoolerr_t read_block(UINT64 block, void *buffer);
		imgtoolerr_t write_block(UINT64 block, const void *buffer);
		imgtoolerr_t clear_block(UINT64 block, UINT8 data);
		imgtoolerr_t list_partitions(imgtool_partition_info *partitions, size_t len);
		void *malloc(size_t size);
		const imgtool_module &module() { return m_module; }
		void *extra_bytes() { return m_extra_bytes; }

	private:
		const imgtool_module &m_module;
		object_pool *m_pool;
		void *m_extra_bytes;

		// because of an idiosycracy of how imgtool::image::internal_open() works, we are only "okay to close"
		// by invoking the module's close function once internal_open() succeeds.  the long term solution is
		// better C++ adoption (e.g. - std::unique_ptr<>, std:move() etc)
		bool m_okay_to_close;

		static imgtoolerr_t internal_open(const imgtool_module *module, const char *fname,
			int read_or_write, util::option_resolution *createopts, imgtool::image::ptr &outimg);
	};
}

namespace imgtool
{
	// ----- partition management -----
	class partition
	{
		friend class directory;
	public:
		typedef std::unique_ptr<partition> ptr;

		// ctor/dtor
		partition(imgtool::image &image, imgtool_class &imgclass, int partition_index, UINT64 base_block, UINT64 block_count);
		~partition();

		static imgtoolerr_t open(imgtool::image &image, int partition_index, ptr &partition);
		image &image() { return m_image; }

		// ----- partition operations -----
		imgtoolerr_t get_directory_entry(const char *path, int index, imgtool_dirent &ent);
		imgtoolerr_t get_file_size(const char *filename, UINT64 &filesize);
		imgtoolerr_t get_free_space(UINT64 &sz);
		imgtoolerr_t read_file(const char *filename, const char *fork, imgtool_stream *destf, filter_getinfoproc filter);
		imgtoolerr_t write_file(const char *filename, const char *fork, imgtool_stream *sourcef, util::option_resolution *resolution, filter_getinfoproc filter);
		imgtoolerr_t get_file(const char *filename, const char *fork, const char *dest, filter_getinfoproc filter);
		imgtoolerr_t put_file(const char *newfname, const char *fork, const char *source, util::option_resolution *opts, filter_getinfoproc filter);
		imgtoolerr_t delete_file(const char *fname);
		imgtoolerr_t list_file_forks(const char *path, imgtool_forkent *ents, size_t len);
		imgtoolerr_t create_directory(const char *path);
		imgtoolerr_t delete_directory(const char *path);
		imgtoolerr_t list_file_attributes(const char *path, UINT32 *attrs, size_t len);
		imgtoolerr_t get_file_attributes(const char *path, const UINT32 *attrs, imgtool_attribute *values);
		imgtoolerr_t put_file_attributes(const char *path, const UINT32 *attrs, const imgtool_attribute *values);
		imgtoolerr_t get_file_attribute(const char *path, UINT32 attr, imgtool_attribute &value);
		imgtoolerr_t put_file_attribute(const char *path, UINT32 attr, const imgtool_attribute &value);
		void         get_attribute_name(UINT32 attribute, const imgtool_attribute *attr_value, char *buffer, size_t buffer_len);
		imgtoolerr_t get_icon_info(const char *path, imgtool_iconinfo *iconinfo);
		imgtoolerr_t suggest_file_filters(const char *path, imgtool_stream *stream, imgtool_transfer_suggestion *suggestions, size_t suggestions_length);
		imgtoolerr_t get_block_size(UINT32 &length);
		imgtoolerr_t read_block(UINT64 block, void *buffer);
		imgtoolerr_t write_block(UINT64 block, const void *buffer);
		imgtoolerr_t get_chain(const char *path, imgtool_chainent *chain, size_t chain_size);
		imgtoolerr_t get_chain_string(const char *path, char *buffer, size_t buffer_len);
		imgtool_partition_features get_features() const;
		void *       get_info_ptr(UINT32 state);
		const char * get_info_string(UINT32 state);
		UINT64       get_info_int(UINT32 state);
		void *       extra_bytes();

		// ----- path management -----
		const char * get_root_path();
		const char * path_concatenate(const char *path1, const char *path2);
		const char * get_base_name(const char *path);

	private:
		imgtool::image &m_image;
		int m_partition_index;
		UINT64 m_base_block;
		UINT64 m_block_count;

		imgtool_class m_imgclass;
		size_t m_partition_extra_bytes;
		size_t m_directory_extra_bytes;

		char m_path_separator;
		char m_alternate_path_separator;

		unsigned int m_prefer_ucase : 1;
		unsigned int m_supports_creation_time : 1;
		unsigned int m_supports_lastmodified_time : 1;
		unsigned int m_supports_bootblock : 1;            /* this module supports loading/storing the boot block */

		imgtoolerr_t(*m_begin_enum)   (imgtool::directory *enumeration, const char *path);
		imgtoolerr_t(*m_next_enum)    (imgtool::directory *enumeration, imgtool_dirent *ent);
		void(*m_close_enum)   (imgtool::directory *enumeration);
		imgtoolerr_t(*m_free_space)   (imgtool::partition *partition, UINT64 *size);
		imgtoolerr_t(*m_read_file)    (imgtool::partition *partition, const char *filename, const char *fork, imgtool_stream *destf);
		imgtoolerr_t(*m_write_file)   (imgtool::partition *partition, const char *filename, const char *fork, imgtool_stream *sourcef, util::option_resolution *opts);
		imgtoolerr_t(*m_delete_file)  (imgtool::partition *partition, const char *filename);
		imgtoolerr_t(*m_list_forks)   (imgtool::partition *partition, const char *path, imgtool_forkent *ents, size_t len);
		imgtoolerr_t(*m_create_dir)   (imgtool::partition *partition, const char *path);
		imgtoolerr_t(*m_delete_dir)   (imgtool::partition *partition, const char *path);
		imgtoolerr_t(*m_list_attrs)   (imgtool::partition *partition, const char *path, UINT32 *attrs, size_t len);
		imgtoolerr_t(*m_get_attrs)    (imgtool::partition *partition, const char *path, const UINT32 *attrs, imgtool_attribute *values);
		imgtoolerr_t(*m_set_attrs)    (imgtool::partition *partition, const char *path, const UINT32 *attrs, const imgtool_attribute *values);
		imgtoolerr_t(*m_attr_name)    (UINT32 attribute, const imgtool_attribute *attr, char *buffer, size_t buffer_len);
		imgtoolerr_t(*m_get_iconinfo) (imgtool::partition *partition, const char *path, imgtool_iconinfo *iconinfo);
		imgtoolerr_t(*m_suggest_transfer)(imgtool::partition *partition, const char *path, imgtool_transfer_suggestion *suggestions, size_t suggestions_length);
		imgtoolerr_t(*m_get_chain)    (imgtool::partition *partition, const char *path, imgtool_chainent *chain, size_t chain_size);

		const util::option_guide *m_writefile_optguide;
		std::string m_writefile_optspec;

		std::unique_ptr<UINT8[]> m_extra_bytes;

		// methods
		imgtoolerr_t cannonicalize_path(UINT32 flags, const char **path, char **alloc_path);
		imgtoolerr_t cannonicalize_fork(const char **fork);
		char *normalize_filename(const char *src);
		imgtoolerr_t map_block_to_image_block(UINT64 partition_block, UINT64 &image_block) const;
	};

	// ----- directory management -----
	class directory
	{
	public:
		typedef std::unique_ptr<directory> ptr;

		// ctor/dtor
		directory(imgtool::partition &partition);
		~directory();

		// methods
		static imgtoolerr_t open(imgtool::partition &partition, const char *path, ptr &outenum);
		imgtoolerr_t get_next(imgtool_dirent &ent);

		// accessors
		imgtool::partition &partition() { return m_partition; }
		imgtool::image &image() { return partition().image(); }
		const imgtool_module &module() { return partition().image().module(); }
		void *extra_bytes() { assert(m_extra_bytes); return m_extra_bytes.get(); }

	private:
		imgtool::partition &m_partition;
		std::unique_ptr<UINT8[]> m_extra_bytes;
		bool m_okay_to_close;	// similar wart as what is on imgtool::image
	};
};

/* ----- special ----- */
int imgtool_validitychecks(void);
void unknown_partition_get_info(const imgtool_class *imgclass, UINT32 state, union imgtoolinfo *info);

char *strncpyz(char *dest, const char *source, size_t len);
char *strncatz(char *dest, const char *source, size_t len);
void rtrim(char *buf);

#endif /* IMGTOOL_H */
