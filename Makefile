# Every nested PlatformIO project is identified by its platformio.ini.
PIO_PROJECTS := $(patsubst %/platformio.ini,%,$(wildcard */platformio.ini))
PIO_DIRS     := $(addsuffix /.pio,$(PIO_PROJECTS))

.PHONY: clean-pio list-pio

# Remove PlatformIO build output and downloaded libdeps from all nested projects.
# NOTE: compile_commands.json is tracked in git, so it is left alone on purpose.
clean-pio:
	rm -rf $(PIO_DIRS)

list-pio:
	@printf '%s\n' $(PIO_PROJECTS)
