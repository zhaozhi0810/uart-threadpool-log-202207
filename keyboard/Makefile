#编译所有子目录
SUBDIRS=`ls -d */ | grep -v 'bin' | grep -v 'lib' | grep -v 'include'`

#编译指定子目录
#SUBDIRS=dir1 dir2 dir3

define make_subdir
 @for subdir in $(SUBDIRS) ; do \
 ([ -d $$subdir ] && cd $$subdir && make $1) \
 done;
endef

all:
	$(call make_subdir,all)
 
install :
	$(call make_subdir,install)
 
debug:
	$(call make_subdir,debug)
clean:
	$(call make_subdir,clean) 


