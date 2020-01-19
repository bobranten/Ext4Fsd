for /R %%a in (*.c;*.cpp;*.h;) do astyle.exe --indent=spaces=4 %%a
for /R %%a in (*.c;*.cpp;*.h;DIRS;sources;) do dos2unix %%a 
