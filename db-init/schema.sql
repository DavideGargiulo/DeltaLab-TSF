
CREATE TABLE public.account (
    id integer NOT NULL,
    nickname character varying(50) NOT NULL,
    email character varying(100) NOT NULL,
    password character varying(255) NOT NULL,
    lingua character varying(5) NOT NULL,
    CONSTRAINT account_lingua_check CHECK (((lingua)::text = ANY ((ARRAY['it'::character varying, 'en'::character varying, 'fr'::character varying, 'de'::character varying, 'es'::character varying, 'pt'::character varying, 'nl'::character varying, 'sv'::character varying, 'no'::character varying, 'da'::character varying, 'fi'::character varying, 'pl'::character varying, 'cs'::character varying, 'sk'::character varying, 'hu'::character varying, 'ro'::character varying, 'bg'::character varying, 'el'::character varying, 'hr'::character varying, 'sl'::character varying, 'lt'::character varying, 'lv'::character varying, 'et'::character varying, 'ja'::character varying, 'ru'::character varying, 'zh'::character varying])::text[])))
DROP TABLE public.account;

ALTER SEQUENCE public.account_id_seq OWNED BY public.account.id;

CREATE TABLE public.lobby (
    id character(6) NOT NULL,
    utenti_connessi integer DEFAULT 0,
    rotazione character varying(10) NOT NULL,
    id_accountcreatore character(6) NOT NULL,
    is_private boolean DEFAULT false,
    status character varying(10) DEFAULT 'waiting'::character varying,
    CONSTRAINT lobby_rotazione_check CHECK (((rotazione)::text = ANY (ARRAY[('orario'::character varying)::text, ('antiorario'::character varying)::text]))),
    CONSTRAINT lobby_status_check CHECK (((status)::text = ANY (ARRAY[('waiting'::character varying)::text, ('started'::character varying)::text, ('finished'::character varying)::text])))
DROP TABLE public.lobby;

ALTER TABLE ONLY public.account ALTER COLUMN id SET DEFAULT nextval('public.account_id_seq'::regclass);
ALTER TABLE public.account ALTER COLUMN id DROP DEFAULT;

ALTER TABLE ONLY public.account
    ADD CONSTRAINT account_email_key UNIQUE (email);
ALTER TABLE ONLY public.account DROP CONSTRAINT account_email_key;

ALTER TABLE ONLY public.account
    ADD CONSTRAINT account_nickname_key UNIQUE (nickname);
ALTER TABLE ONLY public.account DROP CONSTRAINT account_nickname_key;

ALTER TABLE ONLY public.account
    ADD CONSTRAINT account_pkey PRIMARY KEY (id);
ALTER TABLE ONLY public.account DROP CONSTRAINT account_pkey;

ALTER TABLE ONLY public.lobby
    ADD CONSTRAINT lobby_pkey PRIMARY KEY (id);
ALTER TABLE ONLY public.lobby DROP CONSTRAINT lobby_pkey;
