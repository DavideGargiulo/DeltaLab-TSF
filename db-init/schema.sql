CREATE TABLE IF NOT EXISTS ACCOUNT (
    ID CHAR(6) PRIMARY KEY,
    nickname VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(100) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    lingua ENUM(
        'it', 'en', 'fr', 'de', 'es', 'pt', 'nl', 'sv', 'no', 'da', 'fi', 
        'pl', 'cs', 'sk', 'hu', 'ro', 'bg', 'el', 'hr', 'sl', 'lt', 'lv', 'et',
        'ja', -- Giapponese
        'ru', -- Russo
        'zh'  -- Cinese (Mandarino)
    ) NOT NULL
);

CREATE TABLE IF NOT EXISTS LOBBY (
    ID CHAR(6) PRIMARY KEY,
    utenti_connessi INT DEFAULT 0,
    rotazione ENUM('orario', 'antiorario') NOT NULL,
    id_accountCreatore CHAR(6) NOT NULL,
    isPrivate BOOLEAN DEFAULT FALSE,
    status ENUM('waiting', 'started', 'finished') DEFAULT 'waiting',
    FOREIGN KEY (id_accountCreatore) REFERENCES ACCOUNT(ID)
        ON DELETE CASCADE
        ON UPDATE CASCADE
);