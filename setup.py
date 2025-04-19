from setuptools import setup, find_packages

setup(
    name="hsdpy",
    version="0.1.0",
    description="Python ctypes bindings for the Hsdlib C library",
    author="Hassan Abedi",
    author_email="hassan.abedi.t+hsdlib@gmail.com",
    url="https://github.com/habedi/hsdlib",
    packages=find_packages(where="python"),
    package_dir={"": "python"},
    include_package_data=True,
    package_data={
        "hsdpy": [
            "libhsd.so",  # Linux
            "libhsd.dylib",  # macOS
            "hsd.dll",  # Windows
        ]
    },
    install_requires=[
        "numpy>=1.21"
    ],
    classifiers=[
        "Programming Language :: Python :: 3",
        "Operating System :: OS Independent",
    ],
    zip_safe=False,
)
