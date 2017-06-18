#include <assert.h>
#include "datatypes/common.h"
#include "file3d/file3d.h"

void
test_errors() {
    file3d *f = f3d_load("i dont exist");
    assert(f->error_code == FILE3D_ERR_FILE_NOT_FOUND);
    f3d_free(f);

    f = f3d_load("/etc/passwd");
    assert(f->error_code == FILE3D_ERR_UNKNOWN_EXTENSION);
    f3d_free(f);
}

void
test_geo() {
    file3d *f = f3d_load("/tmp/cow.geo");
    assert(f->error_code == FILE3D_ERR_NONE);
    assert(f->n_indices == 9468);
    assert(f->n_verts == 1732);
    assert(f->indices[0] == 2);
    f3d_free(f);
}

int
main(int argc, char *argv[]) {
    PRINT_RUN(test_errors);
    PRINT_RUN(test_geo);
}
