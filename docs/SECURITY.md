# Limites implementados

Projetos não são extraídos: leitura de nomes essenciais e assets com hash estrito. Bloqueios: caminhos inesperados, duplicatas, symlinks Unix, arquivos criptografados, tamanhos, CRC/truncamento, JSON profundo, unidades/enumerações/referências inválidas. Hash dos PNGs incorporados deve coincidir com manifest. Limites de textura e DXF anteriores à construção de geometria.

Salvar valida o temporário e preserva backup. Importar/abrir com erro não troca o documento válido. SQL parametrizado; FTS escapa termos. Processo Blender usa argv separado. Não há login, API, telemetria ou comunicação de dados do projeto.

Os decoders de Qt/FreeImage, libzip e OpenCASCADE continuam sendo superfícies de parsing: manter versões atualizadas e ampliar fuzzing antes de distribuir. Não existe alegação de auditoria de segurança completa, fuzzing extensivo ou teste de queda física de energia. `dependency-audit`, clang-tidy e diagnóstico exportável ainda são requisitos de release pendentes.
