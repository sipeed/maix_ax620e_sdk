SEC_SIGN_HEADER_SIZE := 0x400

define Bytes2KBytes
$(shell printf "%dK" $$(($(1)/1024)))
endef

define Bytes2MBytes
$(shell printf "%dM" $$(($(1)/1024/1024)))
endef

define MB2Bytes
$(shell printf '0x%x' "$$((`echo $(1) | sed 's/M//'` * 1024 * 1024))")
endef

define KM2Bytes
$(if $(findstring K,$(1)),$(shell echo $$(($(subst K,,$(1))*1024))),$(shell echo $$(($(subst M,,$(1))*1024*1024))))
endef

define KM2KBytes
$(if $(findstring K,$(1)),$(shell echo $(1)),$(shell echo $$(($(subst M,,$(1))*1024))))
endef

define AddAddressMB
$(shell printf "0x%X" $$(($(1) + $(2)*1048576)))
endef

define SubAddressMB
$(shell printf "0x%X" $$(($(1) - $(2)*1048576)))
endef

define SubAddressB
$(shell printf "0x%X" $$(($(1) - $(2))))
endef

define AddKM2KBytes
$(shell echo $(1) | sed -e 's/K//' -e 's/M/*1024/' | bc | awk '{s+=$$(1)} END {print s}')
endef

define AutoCalculateRest
$(eval total := 0) \
	$(foreach arg,$(wordlist 2,$(words $(1)),$(1)), \
        $(eval arg_value := $(call AddKM2KBytes,$(arg))) \
        $(eval total := $(shell echo $(total) + $(arg_value) | bc)) \
    ) \
$(eval first = $(call KM2KBytes,$(firstword $(1)))) \
$(shell printf "%dK" $$(($(first) - $(total))))
endef

# Calculates the location of each partition in the flash.
define calculate_flash_base
$(shell echo "$(1)" | sed 's/$(2)/*/' | cut -d '*' -f 1 | tr ',' '\n' | awk '\
{\
  if ($$0 ~ /[0-9]*K/) {\
    sub(/K*/, "", $$0);\
    cur = $$0 * 1024;\
    sum += cur;\
  } else if ($$0 ~ /[0-9]*M/) {\
    sub(/M*/, "", $$0);\
    cur = $$0 * 1024 * 1024;\
    sum += cur;\
  }\
}\
END {\
  printf "0x%x", sum - cur;\
}')
endef
