skiboot_dir=$(CURDIR)/talos-skiboot
skiboot_git_repo=https://git.raptorcs.com/git/talos-skiboot
skiboot_revision=9858186353f2203fe477f316964e03609d12fd1d

$(skiboot_dir):
	echo "    Cloning $(project_name) from Git"
	git clone $(skiboot_git_repo) $(skiboot_dir)
	echo "    Checkingout $(skiboot_revision)
	git checkout $(skiboot_revision)

all:
	$(error BUILDING SKIBOOT)

