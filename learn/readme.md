Create earthren-learn environemnt with

```bash
conda env create -f environment.yml
```

command and activate with

```bash
conda activate earthren-learn
```

GDAL is not that easy to install via pip (we need conda environment for that) because of dependencies. I've got following complain

```
Exception: Python bindings of GDAL 3.9.0 require at least libgdal 3.9.0, but 3.4.1 was found
```

on current *Ubuntu 22.04 LTS*.

Or without miniconda setup, install following

```bash
sudo apt install python3-gdal python3-matplotlib python3-xarray python3-rioxarray
```
