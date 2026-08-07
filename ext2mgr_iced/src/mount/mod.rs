//! Mount / unmount, Ext2Srv pipe, Session Manager persist, dead letters.

#[cfg(windows)]
pub mod pipe;
#[cfg(windows)]
pub mod ops;
#[cfg(windows)]
pub mod persist;
#[cfg(windows)]
pub mod dead_letters;
