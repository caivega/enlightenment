#!/bin/bash

################# Usage ##################
# trunk$ ./devs/seoz/build.sh
##########################################

###### Ubuntu >= 12.04 Pre-requisites #######
# sudo apt-get install build-essential automake libtool ccache zlib1g-dev libjpeg-dev libfreetype6-dev libdbus-1-dev liblua5.1-0-dev g++ libxext-dev libxrender-dev libpng12-dev libxrandr-dev libfontconfig1-dev autopoint libxcomposite-dev libxcursor-dev libxdamage-dev libxdmcp-dev libxfixes-dev libxfont-dev  libxi-dev libxinerama-dev libxss-dev libxv-dev libtiff4-dev librsvg2-dev libfribidi-dev libcurl4-openssl-dev libexif-dev libiptcdata0-dev libxml2-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libudev-dev libxcb-shape0-dev libsigc++-2.0-dev byacc libsqlite3-dev python-dev imagemagick libpam0g-dev python-pyparsing cython libcanberra-dev libv8-dev iptables-dev doxygen libxcb-keysyms1-dev libgif-dev libtiff5-dev libxp-dev libxtst-dev
##########################################

############## Manual Build ##############
# for enjoy : lightmediascanner http://lms.garage.maemo.org/
#             ./autogen.sh && make && sudo make install && sudo ldconfig
# for ephysics : Bullet Physics Engine http://bulletphysics.org/wordpress/
#                Build instruction (Korean) : http://seoz.egloos.com/3857085
# for e17 connman module & econnman : http://www.kernel.org/pub/linux/network/connman/connman-1.6.tar.gz
##########################################

set -e
#set -x
shopt -s expand_aliases

export CFLAGS="-O2 -march=native -ffast-math -g3 -W -Wall -Wextra" # -Wshadow"
export CC="ccache gcc"
alias make='make -j6'

export BUILD_ETHUMB_OPTION="PROTO/epdf"
export BUILD_ELM_OPTION="PROTO/emap"
export BUILD_CORE_EFL="edje e_dbus edbus efreet expedite "$BUILD_ETHUMB_OPTION" emotion ethumb "$BUILD_ELM_OPTION" elementary eeze e"
export BUILD_PYTHON_BINDINGS="BINDINGS/python/python-evas BINDINGS/python/python-ecore BINDINGS/python/python-elementary BINDINGS/python/python-edje BINDINGS/python/python-emotion BINDINGS/python/python-e_dbus"
export BUILD_CPP_BINDINGS="BINDINGS/cxx/eflxx BINDINGS/cxx/einaxx BINDINGS/cxx/evasxx BINDINGS/cxx/ecorexx BINDINGS/cxx/edjexx BINDINGS/cxx/elementaryxx"
export BUILD_BINDINGS=$BUILD_PYTHON_BINDINGS" "$BUILD_CPP_BINDINGS" "
export BUILD_E_MODULES="E-MODULES-EXTRA/comp-scale E-MODULES-EXTRA/elfe E-MODULES-EXTRA/engage E-MODULES-EXTRA/everything-shotgun"
export BUILD_ETC="terminology ephysics_tests econnman exactness efx PROTO/shotgun editje PROTO/elev8 PROTO/eyelight ephoto edje_viewer PROTO/azy elmdentica enlil PROTO/emote emprint clouseau PROTO/enna-explorer envision ensure enjoy exquisite rage PROTO/eyesight"
export BUILD_EXAMPLE="EXAMPLES/elementary/calculator EXAMPLES/elementary/converter EXAMPLES/elementary/phonebook EXAMPLES/elementary/sticky-notes"
export BUILD_GAMES="PROTO/etrophy GAMES/efbb"
export BUILD_ETC2="excessive enki espionnage evas_generic_loaders FORMATTING/ecrustify"
export BUILD_WITH_CMAKE="ecrire"

PWD=`pwd`
LOG_WARN_FILE=$PWD"/warnings.txt"
LOG_WARN_TMP_FILE=$PWD"/warnings_tmp.txt"

function create_log()
{
	rm $LOG_WARN_FILE -f
	touch $LOG_WARN_FILE
}

function build_efl()
{
	echo " "
	echo "============ EFL ============"
	pushd efl
		echo "" >> $LOG_WARN_FILE
		echo "[EFL]" >> $LOG_WARN_FILE
		if [ -f Makefile ]; then
			make clean distclean || true
		fi
		./autogen.sh --disable-doc --disable-cpu-sse3
		make 3>&1 1>&2 2>&3 | tee $LOG_WARN_TMP_FILE
		cat $LOG_WARN_TMP_FILE >> $LOG_WARN_FILE
		rm $LOG_WARN_TMP_FILE
		sudo make install
		sudo ldconfig
	popd
}

function build()
{
	build_dir=$1
	autogen_option=$2
	for I in $build_dir; do
  	pushd $I
		echo " "
		echo "============ "$I" ============"
		echo "" >> $LOG_WARN_FILE
		echo "["$I"]" >> $LOG_WARN_FILE
		if [ -f Makefile ]; then
			make clean distclean || true
		fi
		./autogen.sh "$autogen_option"
		make 3>&1 1>&2 2>&3 | tee $LOG_WARN_TMP_FILE
		cat $LOG_WARN_TMP_FILE >> $LOG_WARN_FILE
		rm $LOG_WARN_TMP_FILE
		sudo make install
		sudo ldconfig
  	popd
	done
}

function build_cmake()
{
	build_dir=$1
	for I in $build_dir; do
	pushd $I
		echo ""
		echo "============ "$I" ============"
		echo "" >> $LOG_WARN_FILE
		echo "["$I"]" >> $LOG_WARN_FILE
		mkdir build -p
		pushd build
			cmake ..
			make 2>> $LOG_WARN_FILE 
			sudo make install
		popd
		sudo ldconfig
	popd
	done
}

function uninstall()
{
	build_dir=$1
	for I in $build_dir; do
	pushd $I
		sudo make uninstall
	popd
	done
}

function build_etc()
{
	echo ""
	echo "============ PROTO/ev ============"
	pushd PROTO/ev
		make
		sudo make install
	popd
}

function build_themes()
{
	#efenniht theme
	echo ""
	echo "============ efenniht ============"
	pushd THEMES/efenniht
		make install-home
	popd

	#detour theme (elm)
	echo ""
	echo "============ detour elm ============"
	pushd THEMES/detour-elm
		make clean
		make
		make install
	popd

	#detourious
	echo ""
	echo "============ detourious ============"
	pushd THEMES/detourious
		make
		make install
	popd

	#darkness theme
	echo ""
	echo "============ darkness ============"
	pushd THEMES/darkness
		#./build.sh -r
		./build.sh -i
		pushd elm
			./build.sh
			cp *.edj ~/.elementary/themes
		popd
	popd

	#23oz theme
	echo ""
	echo "============ 23oz ============"
	pushd THEMES/23oz
		./build.sh -i
	popd
}

function build_game_prerequisites()
{
	# geneet for efbb
	echo ""
	echo "============ geneet ============"
	pushd geneet
		sudo python setup.py install
	popd
}

function efl_ctags()
{
	# efl ctags
	echo ""
	echo "=========== TAGS ============"
	rm tags -f
	ctags -h ".h.x.cpp.c" --exclude="*.js" --exclude="*.pxi" -R
}

function e17_restart()
{
	echo ""
	echo "=========== ENLIGHTENMENT RESTART ============"
	enlightenment_remote -restart
}

create_log

build_efl # eina eet eo embryo evas ecore eio
build ephysics --enable-build-tests
build "$BUILD_CORE_EFL" --disable-doc
build "$BUILD_PYTHON_BINDINGS" "--prefix=/usr/local"
#build "$BUILD_CPP_BINDINGS"
build "$BUILD_E_MODULES $BUILD_ETC $BUILD_EXAMPLE "
#build "$BUILD_ETC2"
build_game_prerequisites
build "$BUILD_GAMES"
build_cmake "$BUILD_WITH_CMAKE"
build_etc
build_themes

efl_ctags
e17_restart
