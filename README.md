
About
-----

    This is a branch of the Ext2Fsd project by Matt Wu where I try to implement support for metadata
    checksums and jbd2. I have also updated the project so it can be compiled with Visual Studio 2017
    and Visual Studio 2019. This is work in progress. If you need a stable driver you should get the
    latest official release from http://www.ext2fsd.com. If you want to try this branch you should
    still install the oficial release and then copy this driver over the old in \windows\system32\drivers.
    The current status of the development is that metadata checksums is implemented but there is an
    calculation error in the inode checksums that must be investigated before I start with jbd2.
    Bo Branten.


Introduction
------------

    Ext3Fsd is an ext2/3/4 file system driver for Windows (2K/2003/XP/7/8/10).
    It's a free and open-source software, everyone can modify or distribute under GNU GPLv2.

    
Development Website
-------------------

    Matt Wu <mattwu@163.com>
    http://www.ext2fsd.com


Active Developers
-----------------

    Matt Wu : http://github.com/matt-wu
              http://blog.dynox.cn

    KaHo Ng : http://github.com/ngkaho1234


Supported Features by Ext3Fsd
-----------------------------

    1, flexible inode size: > 128 bytes, up to block size
    2, dir_index:    htree directory index
    3, filetype:     extra file mode in dentry
    4, large_file:   > 4G files supported
    5, sparse_super: super block backup in group descriptor
    6, uninit_bg:    fast fsck and group checksum
    7, extent:       full support with extending and shrinking.
    8, journal:      only support replay for internal journal
    9, flex_bg:      first flexible metadata group
    10, symlink and hardlink
    11, mount-as-user: specifed uid/gid by user


Unsupported Ext3/4 Features
---------------------------

    1, journal: log-based operations, external journal
    2, EA (extended attributes), ACL support
