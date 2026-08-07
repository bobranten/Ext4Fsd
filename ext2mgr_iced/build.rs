fn main() {
    let mut resource = winres::WindowsResource::new();
    resource.set_icon("assets/Ext2Mgr.ico");
    resource.set_manifest_file("app.manifest");
    // Keep FileVersion / ProductVersion in sync with Cargo.toml `[package].version`.
    let pkg = std::env::var("CARGO_PKG_VERSION").unwrap_or_else(|_| "0.0.0".to_string());
    let file_ver = format!("{}.0", pkg);
    resource.set("FileVersion", &file_ver);
    resource.set("ProductVersion", &file_ver);
    resource.set("ProductName", "Ext2 Volume Manager");
    resource.set("FileDescription", "Ext2/Ext4 volume manager (Iced)");
    if let Err(err) = resource.compile() {
        println!("cargo:warning=winres failed: {err}");
    }
}
