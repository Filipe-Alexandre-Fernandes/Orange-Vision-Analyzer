# Orange Vision Analyzer

**Deteção, contagem e classificação de laranjas em vídeo, com C, C++ e OpenCV.**

Projeto académico desenvolvido na unidade curricular de **Visão por Computador**, do 2.º ano da Licenciatura em Engenharia de Sistemas Informáticos, na Escola Superior de Tecnologia do **Instituto Politécnico do Cávado e do Ave (IPCA)**, no ano letivo **2025/2026**.

O objetivo é analisar cada frame de um vídeo de laranjas e apresentar, sobre a imagem, a localização e as características de cada fruto, a contagem acumulada e uma estimativa do calibre e da categoria de qualidade. A solução usa técnicas clássicas de processamento de imagem, sem modelos de aprendizagem automática.

## Contexto do trabalho

O enunciado do trabalho propõe o processamento de `video.avi`, com resolução de **1280 × 720** e **25 fps**, usando a referência de **280 píxeis = 55 mm**. Os objetivos incluem contagem, área, perímetro, centro de gravidade, caixa delimitadora, calibre e categoria de cada laranja.

O trabalho privilegia a implementação própria das operações de processamento e restringe o uso de funções adicionais de OpenCV. Neste projeto, a biblioteca em C implementa a conversão de cor, a segmentação, a morfologia, a etiquetagem e a análise dos objetos. O OpenCV assegura a leitura do vídeo, as janelas e a apresentação de texto.

## Funcionalidades

| Funcionalidade | Implementação |
| --- | --- |
| Deteção de laranjas | Segmentação HSV e filtragem por área, circularidade e proporção da caixa delimitadora |
| Limpeza da máscara | Abertura e fecho morfológicos com kernel de 3 × 3 |
| Localização | Caixa delimitadora e cruz no centro de gravidade |
| Medições | Área, perímetro e diâmetro estimado, convertidos para unidades físicas |
| Contagem | Número de laranjas na frame e total acumulado com associação temporal dos centros |
| Calibre | Consulta de uma tabela de 14 calibres, de 0 a 13 |
| Categoria | Estimativa por circularidade, defeito de forma e percentagem de defeito de cor |
| Homogeneidade | Comparação entre os diâmetros mínimo e máximo observados em frutos não cortados |
| Visualização | Vídeo anotado e máscara binária em duas janelas |

## Como funciona

1. Lê uma frame BGR e converte-a para HSV através da biblioteca `vc`.
2. Cria duas máscaras: uma para detetar os frutos e outra, mais restritiva, para identificar a cor considerada adequada.
3. Aplica abertura e fecho à máscara de deteção e identifica componentes ligados (*blobs*).
4. Filtra os objetos e calcula área, perímetro, centro, dimensões e descritores de forma.
5. Estima o diâmetro pela média da largura e da altura da caixa delimitadora e converte as medidas com a escala fornecida.
6. Atribui calibre e categoria, associa os centros às deteções anteriores e atualiza as contagens.
7. Desenha as medições e as estatísticas sobre o vídeo.

A associação temporal aceita o primeiro centro anterior disponível a uma distância inferior a **70 píxeis**. Deteções ausentes são conservadas em memória durante até **15 frames**, reduzindo recontagens em falhas breves de deteção.

### Parâmetros principais

Os valores estão definidos no início de `Laranjas.cpp`. Os limites de H são indicados em graus e os de S e V em percentagem; a biblioteca converte-os para a representação interna de 8 bits.

| Parâmetro | Valor |
| --- | --- |
| Máscara de deteção | H: 10–35°; S: 45–100%; V: 18–100% |
| Máscara de cor adequada | H: 13–32°; S: 53–100%; V: 20–100% |
| Área mínima aceite | Superior a 2000 px² |
| Circularidade mínima | 0,50 |
| Proporção mínima entre o menor e o maior lado | 70% |
| Margem para detetar frutos cortados pela imagem | 15 px |
| Escala de referência | 280 px / 55 mm |

### Critérios de classificação implementados

As condições de cada linha são cumulativas e avaliadas pela ordem apresentada:

| Categoria | Circularidade mínima | Defeito de forma máximo | Defeito de cor máximo |
| --- | --- | --- | --- |
| Extra | 0,85 | 5% | 4% |
| I | 0,75 | 12% | 10% |
| II | 0,60 | 25% | 25% |

Um fruto junto à margem é marcado como `Nao classificada`; um diâmetro sem correspondência na tabela recebe `Fora calibre`. Os restantes frutos que não satisfazem os critérios recebem `Rejeitada`.

O defeito de forma é calculado a partir da razão entre o menor e o maior lado da caixa delimitadora. O defeito de cor corresponde à percentagem de píxeis do objeto que não pertencem à máscara de cor adequada. A circularidade é calculada por `4π × área / perímetro²`.

Os intervalos de calibre sobrepõem-se; `get_caliber()` devolve o primeiro intervalo compatível, percorrendo a tabela de 0 a 13. A homogeneidade usa tolerâncias de 11, 9 ou 7 mm, consoante o calibre atribuído ao maior diâmetro observado.

## Estrutura do repositório

| Ficheiro | Responsabilidade |
| --- | --- |
| [Laranjas.cpp](Laranjas.cpp) | Aplicação, processamento das frames, rastreamento e apresentação dos resultados |
| [vc.c](vc.c) | Biblioteca de processamento e análise de imagem em C |
| [vc.h](vc.h) | Estruturas `IVC` e `OVC` e interface pública da biblioteca |
| [CMakeLists.txt](CMakeLists.txt) | Configuração de compilação para C e C++ com OpenCV |

Os três ficheiros de código fonte são preservados tal como constam do ZIP original. A documentação e a configuração de compilação foram acrescentadas para facilitar a consulta e a utilização do projeto.

## Compilar e executar

### Requisitos

- Compiladores de C11 e C++11 ou posteriores.
- OpenCV 4 com cabeçalhos e bibliotecas de desenvolvimento, incluindo `core`, `highgui`, `imgproc` e `videoio`.
- CMake 3.16 ou posterior e uma ferramenta de compilação compatível.
- Ambiente gráfico para apresentar as janelas.
- O vídeo original `video.avi`, disponibilizado separadamente no âmbito da unidade curricular.

**O vídeo não está incluído neste repositório nem no ZIP de código fornecido.** O enunciado e o anexo com as normas de qualidade dos citrinos não são distribuídos neste repositório.

### Com CMake

Na raiz do repositório:

```bash
cmake -S . -B build
cmake --build build --config Release
```

Se o CMake não encontrar o OpenCV, indica a pasta que contém `OpenCVConfig.cmake`:

```bash
cmake -S . -B build -DOpenCV_DIR="/caminho/para/a/configuracao/opencv"
```

Coloca `video.avi` na raiz do repositório e executa a partir dessa pasta:

```bash
# Linux / macOS, com um gerador de configuração única
./build/orange_vision
```

```powershell
# Windows, com o gerador Visual Studio
.\build\Release\orange_vision.exe
```

O caminho do vídeo é relativo à **pasta de trabalho atual**, e não à localização do executável. Na versão original, o nome `video.avi` está definido diretamente no código e não existe um argumento de linha de comandos para escolher outro ficheiro.

A aplicação abre as janelas `DETECAO DE LARANJAS` e `MASCARA BINARIA`. Pressiona **q** com uma das janelas ativa para terminar, ou aguarda o fim do vídeo.

### Compilação direta em Linux

Com GCC, G++ e o pacote de desenvolvimento do OpenCV disponível através de `pkg-config`:

```bash
mkdir -p build
gcc -std=c11 -O2 -c vc.c -o build/vc.o
g++ -std=c++11 -O2 Laranjas.cpp build/vc.o -o build/orange_vision $(pkg-config --cflags --libs opencv4)
./build/orange_vision
```

`vc.c` deve ser compilado como C; a aplicação C++ usa `extern "C"` para aceder à biblioteca.

## Limitações e validação

- Os limiares de cor, os filtros e a escala estão ajustados ao cenário do vídeo do trabalho. Alterações de iluminação, distância, fundo ou resolução exigem nova calibração.
- O calibre e a categoria são estimativas obtidas de uma imagem 2D. Os critérios numéricos acima são heurísticas implementadas no projeto e não demonstram, por si só, conformidade com normas comerciais de citrinos.
- Frutos sobrepostos podem formar um único componente. O rastreamento por proximidade pode perder correspondências ou voltar a contar frutos em movimentos rápidos e oclusões prolongadas.
- A homogeneidade acumula extremos de medições ao longo das frames; não calcula uma média por fruto nem mantém identificadores permanentes.
- O processamento é apresentado à medida que as frames são lidas, com `waitKey(1)`. O valor de FPS exibido é o metadado do vídeo, não uma medição do desempenho da aplicação.
- Na preparação desta publicação, `vc.c` passou uma verificação de sintaxe com `gcc -std=c11 -Wall -Wextra -Wpedantic -fsyntax-only`. A compilação completa e a execução com o vídeo não foram verificadas: o ambiente de preparação não dispunha de OpenCV nem do vídeo original. Não são apresentados resultados de precisão ou desempenho medidos.

## Autores

- **Ana Rita Ranito** — a31511
- **Filipe Fernandes** — a31498

**Docente:** Duarte Duque · **Unidade curricular:** Visão por Computador · **Instituição:** IPCA / Escola Superior de Tecnologia.

Repositório de documentação e preservação de um trabalho académico. A consulta do código deve respeitar as regras de autoria e integridade académica da instituição. Não foi atribuída uma nova licença aos materiais deste repositório.
