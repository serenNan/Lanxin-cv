#!/bin/bash

echo "LxCamera-MRDVS install program!"

# Return 0 for 'n', 1 for 'y'
ChooseYorN() 
{
    local q="$1 (Y/n) : "
    while true; do
        read -p "$q" yn
        case $yn in
            [Yy]* ) return 0; ;;
            [Nn]* ) return 1; ;;
            #* );;
        esac
    done
}

# Make sure script is only being run with root privileges
if [ "$(id -u)" != "0" ] ; 
then
    if ChooseYorN "You don't have root permission, Script will try to use sudo. Sure to continue?" ; 
    then
        SUDO="sudo "
        $SUDO echo ""
    else
        echo "***This script must be run with root privileges!***"
        echo "Script Exit..."
        exit 1;
    fi
fi


echo "move SDK to /opt/Lanxin-MRDVS"
$SUDO rm -rf /opt/Lanxin-MRDVS
$SUDO mkdir /opt/Lanxin-MRDVS
$SUDO cp -r ./SDK/include /opt/Lanxin-MRDVS/
$SUDO mkdir /opt/Lanxin-MRDVS/lib

ARCH=$(uname -m)
echo "arch:" $ARCH
if [ "$(echo $ARCH | grep "arm")" != "" ]; then
	$SUDO cp -r ./SDK/lib/linux_arm32/* /opt/Lanxin-MRDVS/lib/
elif [ "$(echo $ARCH | grep "aarch")" != "" ]; then
	$SUDO cp -r ./SDK/lib/linux_aarch64/* /opt/Lanxin-MRDVS/lib/
elif [ "$(echo $ARCH | grep "x86_64")" != "" ]; then
	$SUDO cp -r ./SDK/lib/linux_x64/* /opt/Lanxin-MRDVS/lib/
else 
	echo "not found matched platform"
	exit
fi

echo ""
echo "export path: /opt/Lanxin-MRDVS/lib/"
export LD_LIBRARY_PATH=/opt/Lanxin-MRDVS/lib/:$LD_LIBRARY_PATH

# Configure for bash
BASH_FILE=~/.bashrc 
if ! grep -Fq "Lanxin-MRDVS" $BASH_FILE; then
	sed -i '$a \export LD_LIBRARY_PATH=/opt/Lanxin-MRDVS/lib/:$LD_LIBRARY_PATH' $BASH_FILE
fi

# Configure for fish
FISH_CONFIG_DIR=~/.config/fish
FISH_CONFIG_FILE=$FISH_CONFIG_DIR/config.fish
if [ ! -d "$FISH_CONFIG_DIR" ]; then
    mkdir -p "$FISH_CONFIG_DIR"
fi
if [ ! -f "$FISH_CONFIG_FILE" ]; then
    touch "$FISH_CONFIG_FILE"
fi

# Add to fish config using fish command
if ! grep -Fq "Lanxin-MRDVS" $FISH_CONFIG_FILE; then
    fish -c "set -U fish_user_paths /opt/Lanxin-MRDVS/lib \$fish_user_paths"
    echo 'if not contains /opt/Lanxin-MRDVS/lib \$LD_LIBRARY_PATH
    set -gx LD_LIBRARY_PATH /opt/Lanxin-MRDVS/lib \$LD_LIBRARY_PATH
end' >> $FISH_CONFIG_FILE
fi

echo ""
echo "set socket buffer size"
$SUDO sh set_socket_buffer_size.sh

echo ""
echo "LxCamera-MRDVS install done!"
