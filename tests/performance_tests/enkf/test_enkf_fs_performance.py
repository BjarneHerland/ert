from res.enkf import EnKFMain, ResConfig


def mount_and_umount(ert, case_name):
    fs_manager = ert.getEnkfFsManager()
    assert not fs_manager.isCaseMounted(case_name)
    mount_point = fs_manager.getFileSystem(case_name)
    assert fs_manager.isCaseMounted(case_name)
    del mount_point
    fs_manager.umount()


def test_mount_fs(benchmark, template_config):
    with template_config["folder"].as_cwd():
        config = ResConfig("poly.ert")
        ert = EnKFMain(config, strict=True)
        benchmark(mount_and_umount, ert, "default")
