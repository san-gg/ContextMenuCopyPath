# ContextCopyPath
This Code Copy text to clipboard.

For linux you need to install libx11-dev (Ubuntu)
```
sudo apt install libx11-dev
```
Checkout respective lib for other distro and install it.

We can add entry to right click context menu and copy the path for files/folder.

![image](https://github.com/san-gg/ContextMenuCopyPath/assets/96869607/fea140d5-be5f-4057-81c5-fcae5d9e322e)

For windows run following command.
```
reg add "HKEY_CLASSES_ROOT\*\shell\Copy Path\command" /t REG_SZ /d "<path of the executable> \"%1\""
```
eg: reg add "HKEY_CLASSES_ROOT\*\shell\Copy Path\command" /t REG_SZ /d "C:\my_programs\ContextCopyPath.exe \\"%1\\""

For Linux refer following links for Ubuntu: -

https://askubuntu.com/questions/1405687/how-to-install-filemanager-actions-in-ubuntu-22-04

https://askubuntu.com/questions/1030940/nautilus-actions-in-18-04

https://askubuntu.com/questions/88480/adding-extra-options-to-right-click-menu/88485#88485
