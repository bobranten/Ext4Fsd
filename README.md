
New
---

    Signed driver for Windows 10 and Windows 11:
    https://www.accum.se/~bosse/ext2fsd/0.71/Ext2Fsd-0.71-setup.exe

    Updated Ext2Mgr https://www.accum.se/~bosse/ext2fsd/next/Ext2Mgr.exe
    that gives more detailed information on the type of filesystems on
    the disk. If an on disk filesystem contains new ext4 features that
    is not supported by the Windows driver it will show a '+' sign after
    the filesystem name, e.g "EXT4+". You can run this application
    together with an already installed driver.


About
-----

    This is a branch of the Ext2Fsd project by Matt Wu where I try to
    implement support for metadata checksums and jbd2. I have also
    updated the project so it can be compiled with Visual Studio 2019
    and Visual Studio 2022.
    The current status of the development is that all metadata checksums
    is implemented and jbd2 is ported to support 64-bit blocknumbers.
    The driver is now ready to be tested!
    This work is dedicated to my mother Berit Ingegerd Branten.
    Bo Branten <bosse@accum.se>


Test
----

    To test this driver run the installation programs:
    Signed driver for Windows 10 and Windows 11:
    https://www.accum.se/~bosse/ext2fsd/0.71/Ext2Fsd-0.71-setup.exe
    Signed driver files for manual install: (even ARM/ARM64)
    https://www.accum.se/~bosse/ext2fsd/0.71/signed/
    Unsigned driver for Windows XP, Windows Vista, Windows 7 and Windows 8:
    https://www.accum.se/~bosse/ext2fsd/0.71/Ext2Fsd-0.71-setup-xp.exe

    If you compile the driver yourself you only need to run the installation
    program once, then you can copy your driver file over the old in
    \windows\system32\drivers.
    Now you can read and write ext4 filesystems using the new features
    metadata checksums and 64-bit blocknumbers from Windows.
    my site: http://www.accum.se/~bosse/


Introduction
------------

    Ext4Fsd is an ext2/3/4 file system driver for Windows (XP/Vista/7/8/10/11).
    It's a free and open-source software, everyone can modify or distribute
    under GNU GPLv2.

    
Old Development Website
-------------------

    Matt Wu <mattwu@163.com>
    http://www.ext2fsd.com


Active Developers
-----------------

    Matt Wu : http://github.com/matt-wu
              http://blog.dynox.cn

    KaHo Ng : http://github.com/ngkaho1234

    Bo Branten : http://github.com/bobranten
                 http://www.accum.se/~bosse

    Thanks to Olof Lagerkvist https://github.com/LTRData
    for important help to this project!


Supported Features by Ext4Fsd
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
