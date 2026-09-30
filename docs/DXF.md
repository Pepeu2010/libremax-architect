# Referência DXF

Arquivo → Importar planta DXF lê o arquivo em worker e apresenta unidade/escala e layers. Detecta $INSUNITS (mm/cm/m/inch/foot); confirmar fator milímetros por unidade. Referência vira entidade GeometryObject por layer, bloqueada, persistente e ocultável. Desbloqueie para editar posição/rotação pelo inspector.

Suporta DXF ASCII: LINE, LWPOLYLINE reta/fechada, POLYLINE+VERTEX+SEQEND, ARC e CIRCLE em XY. Bulge, DXF binário, malhas 3D e INSERT/BLOCK não são suportados. Entidades não suportadas são apresentadas ao usuário; bulge incompleto é erro, evitando deformação silenciosa.

Limite de leitura 8 MiB e 400.000 linhas de grupos. Não há conversão de linhas em paredes nem opacidade por layer. O teste cobre unidade em metros, todos os tipos implementados, layers bloqueados, geometria e roundtrip `.lmx`. O smoke ainda não percorre o diálogo de importação por clicks.
