# LibreMax Architect

Editor desktop open source de interiores, com C++20, Qt 6 e OpenCASCADE. Linux é o alvo principal. A implementação é independente e clean-room, usando o VDMax Arquitetos e Decoradores somente como referência pública de capacidades.

**Estado: 0.3.0 em desenvolvimento. Esta entrega ainda não atende à paridade integral da master spec e não é uma release 1.0.** A [master spec](docs/MASTER_SPEC.md) é o requisito de referência. A [matriz de paridade](docs/VDMAX_ARCHITECT_PARITY.md) registra as lacunas por requisito, sem promover testes do kernel a prova de um workflow completo.

![Interface nativa real, capturada no teste executável](docs/screenshots/native-ui.png)

A direção visual atual usa grafite escuro e destaque verde suave, preservando os fluxos nativos de CAD.

A imagem acima é uma captura do programa compilado no Windows, com viewport OpenCASCADE; não é um mockup. Validação Linux permanece pendente.

Já disponível neste ciclo:

- Desenhar cadeias de paredes e muretas; criar ambiente retangular com piso e forro.
- Inserir portas e janelas com recortes booleanos reais, vinculadas à parede.
- Biblioteca local SQLite/FTS5 com busca sem acentos, filtros, favoritos e recentes; 25 assets procedurais CC0 com miniaturas reais assíncronas e cache local.
- Inserir móveis por duplo clique/drag-and-drop; editar dimensões em milímetros e expressões numéricas, frentes, puxadores e vidro.
- Inspecionar sólidos em planta superior e 3D; orbit, pan, zoom, seleção, ocultação, bloqueio, duplicação e espelhamento de móveis.
- Gerar tampos, rodatampos, rodapés, rodaforros, painel lateral e envelopamento sobre fontes associadas. A cobertura ainda é restrita aos casos documentados em [AUTOMATIONS](docs/AUTOMATIONS.md).
- Importar DXF ASCII com layers e unidade; incorporar texturas JPG/PNG ao projeto.
- Salvar/abrir `.lmx` ZIP versionado, backup `.bak`, undo/redo, autosave configurável (1–60 minutos) e recuperação de versões locais.
- Criar câmeras e luzes point/spot/area; renderizar com Blender/Cycles em processo separado, PNG/JPEG, presets, seleção de câmera, exposição, luz ambiente, denoise e descoberta GPU com fallback CPU.
- Workspace escuro de render com zoom/pan, Ajustar, 1:1 e exportação de cópia PNG/JPEG; materiais PBR incorporados de madeira/pedra, céu natural/sol e câmera fotográfica no Cycles.

Faltam recursos essenciais: detecção de regiões, junções avançadas, cotas, snap completo, escadas L/U, geometria livre/perfis/sancas, importadores 3D, packs, catálogo extenso, agrupamento/gizmos, galeria/fila de renders persistentes e pacotes Linux testados. Veja [ROADMAP](docs/ROADMAP.md).

Para apresentação, abra `examples/cozinha.lmx`, escolha **Céu natural** e a qualidade desejada no Render. Em outros projetos, **Ativar texturas reais** incorpora mapas de carvalho e pedra. Exposição e direção do sol são editáveis; abertura e foco ficam nas propriedades da câmera. [Guia de render](docs/RENDERING.md).

## Compilar e executar

Ubuntu/Mint/Debian com Qt ≥ 6.4:

```bash
bash scripts/bootstrap.sh
bash scripts/build.sh
bash scripts/test.sh
bash scripts/run.sh
```

Não há AppImage ou `.deb` publicado neste ciclo. O bootstrap instala dependências de desenvolvimento e não instala Blender. Escolha o executável Blender no painel Render; o usuário não precisa editar o projeto nele. [Instruções e limites de plataforma](docs/BUILDING.md).

No host Windows desta sessão:

```powershell
./scripts/run-windows.ps1 ./examples/cozinha.lmx
```

## Fluxo de uso atual

Novo projeto → definir ambiente → selecionar parede na árvore → inserir abertura pelo menu Ambiente → arrastar móveis na planta → editar propriedades → selecionar módulos com Ctrl+clique → Automação → criar câmera e luz → painel Render → salvar projeto.

O checkbox **Abrir vista** oculta o forro no viewport e duas orientações de paredes na vista isométrica, facilitando ver o interior. Isso não altera o documento nem o render. Ainda não acompanha automaticamente a órbita.

Em Arquivo, configure o intervalo de autosave ou recupere versões anteriores. São mantidas cinco cópias por projeto; arquivos corrompidos são ignorados e preservados. A recuperação após encerramento forçado foi testada neste host.

| Atalho | Ação |
|---|---|
| Ctrl+N / Ctrl+O | Novo / abrir |
| Ctrl+S / Ctrl+Shift+S | Salvar / salvar como |
| Ctrl+Z / Ctrl+Shift+Z | Desfazer / refazer |
| Ctrl+D / Delete | Duplicar / excluir seleção |
| 1 / 3 | Planta superior / isométrica |
| F | Enquadrar projeto |
| Esc / clique direito durante parede | Encerrar cadeia de desenho |
| Botão do meio / roda | Pan / zoom |
| Botão direito em 3D | Orbit |

[Biblioteca](docs/LIBRARY.md), [materiais](docs/MATERIALS.md), [render](docs/RENDERING.md), [formato do projeto](docs/PROJECT_FORMAT.md), [recuperação](docs/RECOVERY.md), [testes](docs/TEST_REPORT.md), [entrega atual](docs/CYCLE_03.md) e [auditoria das referências visuais](docs/VISUAL_REFERENCE_AUDIT.md).

Código: GPL-3.0-or-later. Receitas e assets próprios: CC0-1.0. Mapas PBR Poly Haven: CC0-1.0, com proveniência em [starter-materials](starter-materials/README.md). [Licenças de dependências](docs/THIRD_PARTY.md). Contribuições devem incluir fluxo integrado e teste; [CONTRIBUTING](docs/CONTRIBUTING.md).
