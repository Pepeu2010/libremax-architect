# Autosave e recuperação

Menu Arquivo → Intervalo do autosave: 1–60 minutos; padrão 5. Preferência persistida localmente. O timer salva apenas documentos com alterações, usando o mesmo container validado e writer atômico do save manual. Cinco versões por UUID, sem sobrescrever o projeto aberto.

Na próxima abertura, ou em Arquivo → Recuperar autosave, a aplicação examina até 100 snapshots recentes. Se houver vários, escolha projeto/versão; confirme a recuperação e salve/ descarte o documento atual quando solicitado. Arquivos corrompidos são ignorados, registrados no log e preservados no disco.

O documento recuperado mantém UUIDs/entidades, recebe sufixo `(recuperado)` e permanece modificado, sem destino de save associado. Salvar o projeto manualmente remove seus snapshots superseded; fechar normalmente após salvar/descartar também limpa suas cópias. Versões de outros projetos são preservadas.

O teste `--recovery-smoke` usa diretório temporário isolado, mata o processo escritor após autosave durável e inicia outro processo que confirma o diálogo. Não muda configurações ou autosaves do usuário. Um projeto foi comprovado pela UI; retenção de dois projetos e skip de corrupção foram comprovados pelo core. Falta de energia e término durante escrita não foram simulados.
