#include <iostream>
#include <gpac/isomedia.h>
#include <gpac/isomedia.h>

#include "../common/util.hpp"

using namespace util;

GF_Err get_isom_udta_str(GF_ISOFile *file, uint32_t dump_udta_type, uint32_t dump_udta_track, char** data_out)
{
	uint8_t *data = nullptr;
	FILE *t = nullptr;
	uint8_t uuid[16];
	uint32_t count = 0;
	GF_Err e;

	memset(uuid, 0, 16);
	count = gf_isom_get_user_data_count(file, dump_udta_track, dump_udta_type, uuid);
	if (!count) {
		return GF_NOT_FOUND;
	}

	count = 0;
	e = gf_isom_get_user_data(file, dump_udta_track, dump_udta_type, uuid, 0, &data, &count);
	if (e) {
		return e;
	}

  defer free_data([&data] { gf_free(data); });

  if ((*data_out) != nullptr) {
    printf("data_out must be NULL\n");
    return GF_BAD_PARAM;
  }

  *data_out = (char *)gf_malloc(count-7);
  if (!*data_out) {
    return GF_OUT_OF_MEM;
  }

  memcpy(*data_out, data + 8, count-8);
  (*data_out)[count-8] = '\0';

	return GF_OK;
}


int main(int argc, char** argv) {
  GF_Err e;
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return -1;
  }
  GF_ISOFile *file = gf_isom_open(argv[1], GF_ISOM_OPEN_READ, nullptr);
  if (!file) {
    printf("Cannot open file %s\n", argv[1]);
    return -1;
  }

  defer close_file([&file] { gf_isom_close(file); });

  char code[] = "stem";
  uint32_t dump_udta_type = GF_4CC(code[0], code[1], code[2], code[3]);

  char* out = nullptr;
  uint32_t size;

  e = get_isom_udta_str(file, dump_udta_type, 0, &out);
  if (e) {
    printf("Error: %s\n", gf_error_to_string(e));
    return -1;
  }

  defer free_out([&out] { gf_free(out); });

  std::cout << out << std::endl;
  return 0;
}


