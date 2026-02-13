from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class UFanConan(ConanFile):
    name = "UFan"
    version = "0.1"
    package_type = "application"

    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    generators = ("CMakeDeps", "CMakeToolchain")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("quill/11.0.2")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
