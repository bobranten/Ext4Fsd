

Table 6.18. GPT partition types defined by Intel. GUID Type Value
 Description
 
00000000-0000-0000-0000-000000000000
 Unallocated entry 
 
C12A7328-F81F-11D2-BA4B-00A0C93EC93B
 EFI system partition 
 
024DEE41-33E7-11d3-9D69-0008C781F39F
 Partition with DOS partition table inside
 

Microsoft has defined some of the type values that it uses, and they are given in Table 6.19.

Table 6.19. GPT partition types that Microsoft has defined. GUID Type Value
 Description
 
E3C9E316-0B5C-4DB8-817D-f92DF00215AE
 Microsoft Reserved Partition (MRP) 
 
EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
 Primary partition (basic disk) 
 
5808C8AA-7E8F-42E0-85D2-E1E90434CFB3
 LDM metadata partition (dynamic disk) 
 
AF9B60A0-1431-4F62-BC68-3311714A69AD
 LDM data partition (dynamic disk)
 





TODO:

2007/07/09
1, bTemporary or not When loading all driver letters
2, reload or trace the driver letter changes

Next version:

Ext2Mgr:

1, Format tools
7, automatically new-version check and update
8, Memory usage, i/o statistics

2, Graphics in DiskView                                                x
3, Registry volume setttings                                           done
4, Temporary or Permanent mountpoints                                  done
   Exit ext2mgr, thenre-open it or re-load the configuration,
   the settings of "temporary/permanet" will be lost, need
   re-query this feature when loading driver letters.

   IOCTL via MountMgr, instead of partition editing 2006/11/16

5, Install Ext2Mgr as a service                                        done, but worth nothing
6, Cdrom property support                                              done

7, more partition types added
8, RAW disk support (non-partitioned disk)
9, bug in SetGlobalProperty
10, profiling/statistics : performance / memory / operation
11, Keyboard events handling

Ext2Fsd:

1, Vista / Longhorn support (vista is done.)
2, Registry settings per volume


2006/10/14

1, disk map handling (killed, a list is used)
2, buttons (donation) (done)
3, ext2 volume control, ext2fsd service control
4, filedisk 
5, partition magic
6, help documents


2006/10/21

1, service control: start ext2fsd service via a button   (done)
2, modify partition entry type                           (done)
3, drive lettre assignment  (add 'permanent' checkbox )  (done)
