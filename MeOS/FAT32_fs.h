#ifndef FAT32_FS_H_01102016
#define FAT32_FS_H_01102016

#include "types.h"
#include "vfs.h"
#include "MassStorageDefinitions.h"

#define FAT_EOF	0x0FFFFFF8

#pragma pack(push, 1)

struct fat_partition_entry
{
	uint8 current_state;		// active - inactive state
	uint8 start_head;			// partition start head
	uint16 start_cylsect;		// partition start cylinder and sector
	uint8 type;					// partition type
	uint8 end_head;				// partition end head
	uint16 end_cylsect;			// partition end cylinder and sector
	uint32 lba_offset;			// sectors between MBR and partition start
	uint32 size;				// size of partition in sectors
};

struct fat_mbr
{
	uint8 boot_code[446];
	fat_partition_entry primary_partition;
	// TODO: to be changed... i don't know if type is correct
	fat_partition_entry secondary_partition;
	fat_partition_entry other_partition;
	fat_partition_entry another_partition;
	////////////
	uint16 boot_sign;
};

struct fat_ext_volume_id
{
	uint32 sectors_per_FAT;
	uint16 extended_flags;
	uint16 fat_version;
	uint32 root_cluster_lba;
	uint16 fat_info;
	uint16 backup_BS_sector;
	uint8 reserved_0[12];
	uint8 drive_number;
	uint8 reserved_1;
	uint8 boot_signature;
	uint32 volume_id;
	uint8 volume_label[11];
	uint8 fat_type_label[8];
};

struct fat_volume_id
{
	uint8 boot_jmp[3];
	uint8 oem_name[8];
	uint16 bytes_per_sector;
	uint8 sectors_per_cluster;
	uint16 reserved_sector_count;
	uint8 number_FATs;
	uint16 root_entry_count;
	uint16 total_sectors_16;
	uint8 media_type;
	uint16 table_size_16;
	uint16 sectors_per_track;
	uint16 head_side_count;
	uint32 hidden_sector_count;
	uint32 total_sectors_32;

	fat_ext_volume_id extended;
};

struct fat_dir_entry_short
{
	uint8 name[8];
	uint8 extension[3];			// 8.3 name
	uint8 attributes;
	uint8 resv0;
	uint8 created_time_10;		// created time in thenths of a second
	uint16 created_time;		// created time following special format
	uint16 created_date;		// created date following special format
	uint16 last_accessed_date;
	uint16 cluster_high;		// first cluster high bits (mask 4 top bits)
	uint16 last_modified_time;
	uint16 last_modified_date;
	uint16 cluster_low;			// first cluster low bits
	uint32 file_size;			// file size
};

struct fat_dir_entry_long
{
	uint8 order;				// the order in which this long name entry appears
	uint8 name1_5[10];			// first 5 characters each a unicode one
	uint8 attributes;			// attributes same as for short entry but need to be FAT_LFN
	uint8 type;
	uint8 checksum;
	uint8 name6_11[12];			// following 6 characters
	uint16 clustoer_low;		// cluster low always zero for lfn
	uint8 name12_13[4];			// last 2 characters
};

enum FAT_DIR_ATTRIBUTES
{
	FAT_READ_ONLY = 1,
	FAT_HIDDEN = 2,
	FAT_SYSTEM = 4,
	FAT_VOLUME_ID = 8,
	FAT_DIRECTORY = 16,
	FAT_ARCHIVE = 32,
	FAT_LFN = FAT_READ_ONLY | FAT_HIDDEN | FAT_SYSTEM | FAT_VOLUME_ID		// LONG FILE NAME
};

#pragma pack(pop, 1)

typedef list<uint32> fat_file_layout;

struct fat_mount_data
{
	uint32 partition_offset;			// start of the FAT partition where the volume ID is located
	uint32 fat_lba;
	uint32 cluster_lba;
	uint32 root_dir_first_cluster;
};

// mount the FAT32 filesystem using the 'mount_name'.
// returns the pointer to the mount file head
vfs_node* fat_fs_mount(char* mount_name, mass_storage_info* info);

// loads the file's, pointed by 'node', cluster chain
void fat_fs_load_file_layout(fat_mount_data* mount_info, mass_storage_info* storgae_info, vfs_node* node);

// loads a page (4KB) at specified address
int fat_fs_load_file(vfs_node* mount_point, mass_storage_info* storage_info, vfs_node* node,
	uint32 file_page, virtual_addr address);

#endif