# Patch to convert old-style
# definitions to new-style

s/sint8/int8_t/g
s/uint8\([^_]\)/uint8_t\1/g
s/sint16/int16_t/g
s/uint16\([^_]\)/uint16_t\1/g
s/sint32/int32_t/g
s/uint32\([^_]\)/uint32_t\1/g
s/sint64/int64_t/g
s/uint64\([^_]\)/uint64_t\1/g

