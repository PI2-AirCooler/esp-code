# AcquaCooler
Repositório referente ao código fonte software embarcado do AcquaCooler, trabalho desenvolvido para a disciplina de Projeto Integrador 2 da Universidade de Brasília.

## Instalação

O processo de instalação abaixo parte do princípio que o sistema operacional utilizado é Linux. Alguns passos podem ser diferentes caso outro sistema operacional seja usado. 

O primeiro passo consiste em clonar o repositório do projeto [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html), framework utilizado no desenvolvimento do software embarcado:

```
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
```

Depois é necessário executar o script de instalação que está dentro do repositório clonado:

```
cd ~/esp/esp-idf
./install.sh all
```

E então configurar as variáveis de ambiente: 

```
. $HOME/esp/esp-idf/export.sh
```

Vale salientar que o comando acima deve ser executado toda vez em que se for utilizar o framework esp-idf. 

## Execução

Na sequência, é necessário conectar a ESP32 utilizando um cabo USB, e depois configurá-la com os seguintes comandos:

```
cd ~/esp/hello_world
idf.py set-target esp32
idf.py menuconfig
```

Uma vez que o dispositivo tenha sido configurado, é necessário construir a imagem que será instalada no microcontrolador:

```
idf.py build
```

E, por fim, enviar a imagens para o dispositivo:

```
idf.py -p PORT
```

Onde PORT é a porta em que o dispositivo está conectada. A documentação da esp-idf indica como descobrir esse [valor](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#connect-your-device).
