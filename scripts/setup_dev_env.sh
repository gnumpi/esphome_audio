#!/bin/bash
venv=".venv"
esphome=".esphome"
PYTHON=python3

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

# Clone esphome
git clone https://github.com/esphome/esphome.git "${esphome}"
pip3 install -e "${esphome}"

pre-commit install
