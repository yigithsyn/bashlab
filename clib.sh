# install libcurl
sudo apt-get install libcurl4-gnutls-dev -qq
# clone
git clone https://github.com/clibs/clib.git /tmp/clib && cd /tmp/clib
git checkout 04d273f1481534c400336bfbbe45d438a52a51a3
# build
make
# put on path
sudo make install
# install libraries
# clib install littlstar/b64.c 


