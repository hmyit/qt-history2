TEMPLATE = subdirs
SUBDIRS = listnames \
	  pingpong \
	  complexpingpong \
	  chat \
	  remotecontrolledcar

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qdbus
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qdbus
INSTALLS += sources
