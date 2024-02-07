#!/bin/bash
PYTHON=python3
venv=".venv"
esphome=".esphome_repo"
espadf=".esp-adf"

if [ ! -d "${venv}" ]; then
    echo "Creating virtual environment at ${venv}"
    rm -rf "${venv}"
    "${PYTHON}" -m venv "${venv}"
    source "${venv}/bin/activate"

else
    source "${venv}/bin/activate"
fi

# Install Python dependencies
echo 'Installing Python dependencies'
pip3 install --upgrade pip
pip3 install -r requirements_dev.txt

if [ ! -d "${esphome}" ]; then
    # Clone esphome
    git clone https://github.com/esphome/esphome.git "${esphome}"
    pip3 install -e "${esphome}"
fi

if [ ! -d "${espadf}" ]; then
    git clone https://github.com/espressif/esp-adf.git .esp-adf
    cd ./esp-adf
    git submodule update --init --recursive
    cd ..
fi

pre-commit install
