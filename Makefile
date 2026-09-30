all := libnd-equip

LDLIBS-libnd-equip := -lxylem -lm

# Sibling -I for a dev build. In CI the siblings do not exist and all headers
# come from the installed packages named in .github/workflows/ci.yml.
CFLAGS += -I$(shell cd .. && pwd)/axil-nd/include
CFLAGS += -I$(shell cd .. && pwd)/axil-nd-attr/include

FOLDER := nd

-include ./../mk/include.mk
