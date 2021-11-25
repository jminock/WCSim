# Download base image from cern repo on docker hub
FROM cern/cc7-base:latest

### Run the following commands as super user (root):
USER root

### install prerequisites
RUN yum install -y cmake \
                   binutils \
                   make \
                   git \
                   tar \
                   wget \
                   devtoolset-3-gcc \
                   devtoolset-3-gcc-c++ \
                   which \
                   expat-devel \
                   xerces-c-devel \
                   libX11-devel \
                   libXmu-devel \
                   libGL-devel \
                   libGLU-devel \
                   libXpm-devel \
                   libXft \
                   libXft-devel \
                   osg-wn-client \
&& yum clean all \
&& rm -rf /var/cache/yum

# since we installed g++ from devtoolset (because we need proper c++11 support)
# it doesn't automatically set up links in /usr/local/bin for us, so do that manually.
RUN for file in /opt/rh/devtoolset-3/root/usr/bin/{g*,c*}; do \
   fname=$(basename $file); \
   ln -s $file /usr/bin/$fname; \
done

# make a home area to install stuff in
RUN mkdir -p /home/annie

# build Geant
RUN mkdir /home/annie/Geant4 && \
    cd /home/annie/Geant4 && \
    wget http://cern.ch/geant4-data/releases/geant4.10.01.p03.tar.gz && \
    tar -xzf geant4.10.01.p03.tar.gz && \
    rm geant4.10.01.p03.tar.gz && \
    mkdir /home/annie/Geant4/install /home/annie/Geant4/build && \
    cd /home/annie/Geant4/build && \
    cmake -DCMAKE_INSTALL_PREFIX=/home/annie/Geant4/install -DGEANT4_INSTALL_DATADIR=/home/annie/Geant4/install/share/data -DGEANT4_BUILD_MULTITHREADED=ON -DGEANT4_BUILD_CXXSTD=c++11 -DGEANT4_USE_GDML=ON -DGEANT4_USE_OPENGL_X11=ON -DGEANT4_INSTALL_DATA=ON /home/annie/Geant4/geant4.10.01.p03 -DGEANT4_INSTALL_DATA=ON && \
    make -j8 && \
    make install && \
    rm -r /home/annie/Geant4/geant4.10.01.p03 && \
    rm -r /home/annie/Geant4/build

# build ROOT
RUN mkdir /home/annie/ROOT && \
    cd /home/annie/ROOT && \
    wget https://root.cern/download/root_v6.06.08.source.tar.gz && \
    tar -xzf root_v6.06.08.source.tar.gz && \
    rm root_v6.06.08.source.tar.gz && \
    mkdir /home/annie/ROOT/install /home/annie/ROOT/build && \
    cd /home/annie/ROOT/build && \
    cmake -DCMAKE_INSTALL_PREFIX=/home/annie/ROOT/install -Dcxx11=ON -Dmt=ON -Dx11=ON -DCMAKE_BUILD_TYPE=Rel /home/annie/ROOT/root-6.06.08 && \
    make -j8 && \
    make install && \
    rm -r /home/annie/ROOT/root-6.06.08 && \
    rm -r /home/annie/ROOT/build

# TODO - install GENIE? maybe v3?

# build WCSim
RUN mkdir /home/annie/WCSim && \
    cd /home/annie/WCSim && \
    git clone https://github.com/ANNIEsoft/WCSim.git WCSim && \
    cd /home/annie/WCSim/WCSim && \
    git checkout annie && \
    source /home/annie/Geant4/install/bin/geant4.sh && \
    source /home/annie/ROOT/install/bin/thisroot.sh && \
    make rootcint && \
    make
RUN mkdir /home/annie/WCSim/build && \
    cd /home/annie/WCSim/build && \
    export NO_GENIE=1 && \
    source /home/annie/Geant4/install/bin/geant4.sh && \
    source /home/annie/ROOT/install/bin/thisroot.sh && \
    cmake ../WCSim && \
    make -j8

RUN echo "#!/bin/bash"                                      > /home/annie/WCSim/WCSim/sourceme  && \
    echo "source /home/annie/Geant4/install/bin/geant4.sh" >> /home/annie/WCSim/WCSim/sourceme  && \
    echo "source /home/annie/ROOT/install/bin/thisroot.sh" >> /home/annie/WCSim/WCSim/sourceme  && \
    echo "source /home/annie/WCSim/WCSim/envHadronic.sh"   >> /home/annie/WCSim/WCSim/sourceme  && \
    echo "export NO_GENIE=1"                               >> /home/annie/WCSim/WCSim/sourceme  && \
    echo 'export LD_LIBRARY_PATH=/home/annie/WCSim/WCSim:$LD_LIBRARY_PATH' >> /home/annie/WCSim/WCSim/sourceme && \
    echo 'export ROOT_INCLUDE_PATH=/home/annie/WCSim/WCSim/include:$ROOT_INCLUDE_PATH' >> /home/annie/WCSim/WCSim/sourceme

RUN echo "#!/bin/bash"                                                                >  /home/annie/setup  && \
    echo "PS1='\[\033[00;32m\]\u@WCSim\[\033[00m\]:\[\033[00;36m\]\w\[\033[00m\]\$ '" >> /home/annie/setup  && \
    echo "source /home/annie/WCSim/WCSim/sourceme"                                    >> /home/annie/setup  && \
    echo "export LIBGL_ALWAYS_INDIRECT=1"                                             >> /home/annie/setup  && \
    echo "alias root='root -l'"                                                       >> /home/annie/setup  && \
    echo "alias cp='cp -i'"                                                           >> /home/annie/setup  && \
    echo "alias mv='mv -i'"                                                           >> /home/annie/setup  && \
    echo "alias diff='diff -s'"                                                       >> /home/annie/setup

# also requested for grid job use: create a bindpoint for /cvmfs
# (grid jobs automatically try to bind this)
RUN mkdir /cvmfs

### Open terminal
WORKDIR /home/annie/WCSim
ENTRYPOINT ["/bin/bash", "--rcfile", "/home/annie/setup"]
#CMD ["/home/annie/WCSim/build/WCSim"]
