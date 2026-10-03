# Teste no notebook e comparação com VDMax

Use o notebook i3-6006U, Intel HD Graphics 520, 8 GB conectado à tomada. Feche
outros programas e instale a prévia 0.15. O Blender já acompanha o instalador.

No menu Iniciar, abra **LibreMax Architect → Testar notebook**. O editor abrirá
sozinho, fará operações de montagem e produzirá três imagens pequenas com o
processador. Aguarde o resultado, que pode levar vários minutos. O teste usa
uma pasta separada e não altera seus projetos.

Os arquivos ficam em **Documentos/LibreMax-Benchmark**. Envie `hardware.json`,
`report.json`, `acceptance.stdout.txt` e `acceptance.stderr.txt`. O primeiro
registra CPU, GPU, driver, RAM e pico de memória do aplicativo e seus processos;
o segundo registra os tempos de edição e o dispositivo OpenGL realmente usado.
Os arquivos não contêm nome do computador nem número de série.

Esse teste mede uma cena controlada de 48 objetos e fotos de 320 × 180. Ainda é
necessário testar um apartamento de uso real e uma foto de 1920 × 1080; uma
aprovação automática não equivale à aprovação de todos os projetos.

Para comparar com VDMax, faça os mesmos três ambientes nos dois programas:
sala de 5 × 4 m, quarto de 3,5 × 3 m e cozinha de 3 × 2,5 m, paredes de 2,7 m.
Coloque sofá, mesa, quatro cadeiras, cama, guarda-roupa, bancada, armários e
geladeira. Use medidas equivalentes quando os catálogos tiverem modelos distintos.

Registre o tempo para criar paredes e aberturas, colocar móveis junto à parede,
mover um conjunto, corrigir uma medida, salvar e reabrir. Anote dificuldades,
travamentos e tentativas de colocar móveis fora do ambiente. Tire capturas das
mesmas vistas. Esses resultados avaliam facilidade de uso, além de velocidade.

Faça uma foto em 1920 × 1080 em cada programa, com enquadramento, materiais e
iluminação semelhantes. No LibreMax, escolha **Processador**, 64 amostras e
redução de ruído. Registre o tempo total mostrado pela fila e a memória pelo
Gerenciador de Tarefas. Guarde as duas imagens. Como motores, modelos e materiais
podem diferir, a comparação de tempo não isola apenas o desempenho do motor.

Critérios: abertura sem erro, montagem sem travamentos, medidas e encaixes
corretos, projeto reaberto sem perda e foto final concluída sem falta de memória.
A qualidade da foto e a facilidade de uso devem ser avaliadas nas imagens e no
fluxo real, antes de afirmar que o aplicativo supera o VDMax.
