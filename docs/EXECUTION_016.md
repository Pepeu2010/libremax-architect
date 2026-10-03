# Execução 0.16 — conjuntos e orientação

## Mudanças integradas

- Giro de 90 graus e espelhamento horizontal de conjuntos e seleções múltiplas,
  sobre um centro comum. A geometria de cada móvel muda; o grupo não recebe uma
  transformação sem efeito nos filhos.
- Validação integral antes da alteração: paredes, outros móveis e limites reais
  do cômodo. Altura, distância relativa e objetos sobre mesas são preservados.
  As peças não sofrem arredondamentos individuais a cada transformação.
- Bloqueio herdado de um conjunto respeitado ao mover ou transformar um filho;
  conjuntos bloqueados não podem ser separados. Histórico preserva a cena exata
  anterior e permite refazer a transformação.
- Tutorial atualizado: 21 assuntos, índice acessível por teclado, diagramas
  originais para contorno em L e giro, janela ajustada à área disponível e
  rolagem vertical. Contorno, importação de seis formatos, coleções, Leve,
  estilos, progresso, galeria e arquivo único possuem orientação específica.
- Linguagem de cálculo: Automático / Processador. Os valores internos AUTO/CPU
  continuam compatíveis. Falha de importação mostra aviso curto com registro
  técnico separado, preservando o projeto.

## Evidência local

Windows, Qt 6.11.2 / OpenCASCADE 7.9.3 / GCC 16.2. Host Ryzen 5 5500,
RX 7600 e 16 GB; não é o notebook mínimo.

`--experience-smoke`: aprovado com navegação nos 21 assuntos, índice por teclado
para o último assunto e de volta à introdução, 680 × 520 sem rolagem horizontal,
ilustrações, conclusão persistente, reabertura, novo cômodo, salvamento/abertura
`.lmx` e biblioteca de projetos. Capturas foram inspecionadas.

`--apartment-tools-smoke`, Blender 4.5.9 incluído: aprovado com desenho nativo do
contorno em L, ajuste de cantos, arraste de conjunto, alinhamento, ações reais
Girar/Espelhar, desfazer e arquivo portátil. Inclui importação OBJ pelo Blender,
instalação de 28 modelos, três imagens Cycles CPU diferentes de 320 × 180 / 8 e
benchmark de 48 cópias. Um OBJ inválido produziu o aviso simples e detalhes
separados, sem alterar o projeto.

O benchmark desta execução mediu 101 ms de carga, p50 43,58 ms e p95 50,86 ms;
439.872 triângulos completos / 71.952 na edição. São tempos da cena controlada,
não FPS nem garantia de melhora no notebook. Há variação entre execuções.

Evidência local: `build-render/experience-016-fixed/` e
`build-render/apartment-016-final/`. O binário deste ciclo está em
`build-render/next/`; o lançador de desenvolvimento o prefere quando existe.

## Gates e limites

Build Release, clang-format 23.1.1 e git diff --check passaram. O núcleo
concluiu 52 testes / 3.065 verificações, incluindo transformação de posições
reais, objetos sobre mesas, histórico, recusa atômica e bloqueios herdados.
A CI Linux e os instaladores 0.16 ainda precisam executar os gates desta fonte;
os links atuais de instalação permanecem na prévia 0.15.0-preview.3.

O espelhamento usa o eixo horizontal da planta; pivô manual, gizmos e encaixe
coletivo automático na parede permanecem pendentes. Colisões usam volumes que
envolvem as peças. O tutorial orienta tarefas e não afirma que o usuário já as
executou. A master spec permanece incompleta. Testes físicos i3/HD 520/8 GB,
comparação VDMax e outras distribuições/GPU continuam abertos.
