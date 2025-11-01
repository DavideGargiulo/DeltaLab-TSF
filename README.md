# Telefono Senza Fili 📞

Un'applicazione client-server multilingua per giocare al classico gioco del telefono senza fili con altri utenti in tempo reale.

## 📋 Descrizione

Telefono Senza Fili è un gioco multiplayer dove i giocatori si alternano nello scrivere frasi, creando una catena di interpretazioni che porta spesso a risultati divertenti e inaspettati. L'applicazione supporta più lingue grazie all'integrazione con LibreTranslate e utilizza WebSocket per la comunicazione in tempo reale.

## 🏗️ Architettura

- **Backend**: Server C con libreria Mongoose per gestione WebSocket
- **Frontend**: Applicazione JavaFX
- **Database**: PostgreSQL per la persistenza dei dati
- **Containerizzazione**: Docker e Docker Compose
- **Traduzione**: Integrazione con API LibreTranslate

## ✨ Funzionalità

- Registrazione e autenticazione utenti
- Creazione e gestione lobby di gioco
- Partecipazione come giocatore attivo o spettatore
- Supporto multilingua con traduzione automatica
- Comunicazione in tempo reale tramite WebSocket
- Gestione automatica delle disconnessioni

## 🚀 Installazione e Avvio

### Prerequisiti

- Docker e Docker Compose
- Java 17 o superiore
- Maven

### Istruzioni

1. **Clona il repository**
   ```bash
   git clone https://github.com/DavideGargiulo/DeltaLab-TSF.git
   cd DeltaLab-TSF
   ```

2. **Avvia i servizi Docker**

   Dalla root del progetto, avvia il backend e il database:
   ```bash
   docker-compose up -d
   ```

3. **Compila il client JavaFX**

   Naviga nella cartella del frontend e compila il progetto:
   ```bash
   cd frontend/tsf
   mvn clean package
   ```

4. **Avvia l'applicazione**

   Esegui il JAR con dipendenze generato:
   ```bash
   java -jar target/tsf-*-jar-with-dependencies.jar
   ```

## 🎮 Come Giocare

1. **Registrati** o effettua il **login** con le tue credenziali
2. **Crea una nuova lobby** oppure **unisciti a una esistente**
3. Attendi che si raggiunga il numero minimo di giocatori (4)
4. Il creatore della lobby può avviare la partita
5. A turno, scrivi la tua frase basandoti su quella precedente
6. Al termine, goditi la sequenza completa di frasi!

## 🛠️ Tecnologie Utilizzate

- **C** - Backend
- **Java** - Frontend
- **JavaFX** - Interfaccia grafica
- **Mongoose** - Gestione WebSocket in C
- **PostgreSQL** - Database
- **Docker** - Containerizzazione
- **Maven** - Gestione dipendenze Java
- **LibreTranslate** - Traduzione multilingua

## 📝 Licenza

Questo progetto è rilasciato sotto licenza MIT. Vedi il file [LICENSE](LICENSE) per maggiori dettagli.

## 👥 Contributors

<table align="center">
  <tr >
    <td align="center">
      <a href="https://github.com/DavideGargiulo">
        <img src="https://github.com/DavideGargiulo.png" width="100px;" alt="DavideGargiulo"/>
        <br />
        <sub><b>Davide Gargiulo</b></sub>
      </a>
      <br />
    </td>
    <td align="center">
      <a href="https://github.com/Franwik">
        <img src="https://github.com/Franwik.png" width="100px;" alt="Franwik"/>
        <br />
        <sub><b>Francesco Donnarumma</b></sub>
      </a>
      <br />
    </td>
    <td align="center">
      <a href="https://github.com/zGenny">
        <img src="https://github.com/zGenny.png" width="100px;" alt="zGenny"/>
        <br />
        <sub><b>Gennaro De Gregorio</b></sub>
      </a>
      <br />
    </td>
  </tr>
</table>

## 🎓 Progetto Universitario

Progetto realizzato per il corso di **Laboratorio di Sistemi Operativi**
Anno Accademico 2024/2025
Università degli Studi di Napoli Federico II