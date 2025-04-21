from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension("hsdpy._platform_marker", sources=[])
    ],
    zip_safe=False
)
