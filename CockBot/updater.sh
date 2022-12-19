#! /bin/dash
#git pull 
echo "Git updated"
OS="`uname`"
case $OS in
        'Linux')
                make clean 
                echo "building"
                make -j$(nproc) &> build.log 
                ;;
        'NetBSD')
        	make -f Makefile.bsd clean 
        	echo "Building"
                make -f Makefile.bsd -j4 
		;;
    esac
    exit $?
fi