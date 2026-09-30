# Automações atuais

Selecionar somente FurnitureModule desbloqueados; escolher menu Automação e espessura/avanço. Fontes são UUIDs persistidos em metadata.sources. A geometria é recalculada a cada reconstrução do viewport/export; undo/redo restaura a associação. Apagar fonte remove dependentes em cascata.

- Tampo: caixas sobre cada módulo com avanço; união booleana quando há contato.
- Rodatampo: segmento traseiro de 100 mm acima do topo.
- Rodapé: painel frontal recuado 60 mm, altura dos pés.
- Rodaforro: segmento frontal acima do topo, altura 100 mm.
- Fechamento: painel lateral direito; ainda não detecta lacunas até parede.
- Envelopamento: dois lados e topo de cada fonte; ainda não envolve um grupo como perímetro único.

Casos testados: sequência reta de dois módulos, crescimento de largura, volume da união e remoção associativa. Não há aprovação de todas as automações em cozinhas reais, cantos/U, alturas incompatíveis ou recortes de cubas. Detecção de módulos adequados para rodaforro, lados configuráveis, perfis e colisões são lacunas explícitas.
