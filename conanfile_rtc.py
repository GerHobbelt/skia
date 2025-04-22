from conans import ConanFile


class SkiaConan(ConanFile):
    name = "skia"
    version = "m98"
    url = "https://github.com/Esri/skia/tree/runtimecore"
    license = "https://github.com/Esri/skia/blob/runtimecore/LICENSE"
    description = "Skia is a complete 2D graphic library for drawing Text, Geometries, and Images."

    # RTC specific triple
    settings = "platform_architecture_target"

    def package(self):
        base = self.source_folder + "/"
        relative = "3rdparty/skia/"

        # headers
        self.copy("*.h*", src=base + "include/codec", dst=relative + "include/codec")
        self.copy("*.h*", src=base + "include/config", dst=relative + "include/config")
        self.copy("*.h*", src=base + "include/core", dst=relative + "include/core")
        self.copy("*.h*", src=base + "include/effects", dst=relative + "include/effects")
        self.copy("*.h*", src=base + "include/encode", dst=relative + "include/encode")
        self.copy("*.h*", src=base + "include/gpu", dst=relative + "include/gpu")
        self.copy("*.h*", src=base + "include/private", dst=relative + "include/private")
        self.copy("skcms.h", src=base + "include/third_party/skcms", dst=relative + "include/third_party/skcms")

        self.copy("*.h*", src=base + "src/core", dst=relative + "src/core")
        self.copy("*.h*", src=base + "src/shaders", dst=relative + "src/shaders")
        self.copy("*.h*", src=base + "src/utils", dst=relative + "src/utils")

        # libraries
        output = "output/" + str(self.settings.platform_architecture_target) + "/staticlib"
        self.copy("*" + self.name + "*", src=base + "../../" + output, dst=output)
