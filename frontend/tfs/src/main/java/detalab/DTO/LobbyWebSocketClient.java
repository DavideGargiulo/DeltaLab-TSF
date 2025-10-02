package detalab.DTO;

import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONObject;
import org.json.JSONArray;

import java.net.URI;
import java.util.concurrent.*;
import java.util.function.Consumer;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Client WebSocket asincrono per la lobby multiplayer.
 * Gestisce connessione, invio/ricezione messaggi e callback per eventi specifici.
 */
public class LobbyWebSocketClient extends WebSocketClient {

    // Listeners per eventi specifici
    private final Map<String, Consumer<JSONObject>> messageHandlers = new ConcurrentHashMap<>();
    private Consumer<String> onConnectionOpen;
    private Consumer<String> onConnectionClose;
    private Consumer<Exception> onError;

    // Executor per gestione asincrona
    private final ExecutorService executor = Executors.newCachedThreadPool();
    private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);

    // Stato connessione
    private volatile boolean isConnected = false;
    private String lobbyId;
    private String playerId;
    private String username;

    /**
     * Costruttore del client WebSocket
     * @param serverUri URI del server (es: ws://localhost:8080/ws/lobby/default)
     */
    public LobbyWebSocketClient(String serverUri) throws Exception {
        super(new URI(serverUri));
        setupDefaultHandlers();
    }

    /**
     * Costruttore con parametri lobby
     * @param host Host del server (es: localhost)
     * @param port Porta del server
     * @param lobbyId ID della lobby
     */
    public LobbyWebSocketClient(String host, int port, String lobbyId) throws Exception {
        super(new URI(String.format("ws://%s:%d/ws/lobby/%s", host, port, lobbyId)));
        this.lobbyId = lobbyId;
        setupDefaultHandlers();
    }

    /**
     * Setup handler di default per i messaggi del server
     */
    private void setupDefaultHandlers() {
        // Handler per messaggio di benvenuto
        onMessageType("welcome", msg -> {
            this.lobbyId = msg.optString("lobbyId");
            System.out.printf("[WS] Connesso alla lobby %s - Giocatori: %d/%d%n",
                msg.optString("lobbyId"),
                msg.optInt("players"),
                msg.optInt("maxPlayers"));
        });

        // Handler per join riuscita
        onMessageType("join_success", msg -> {
            this.playerId = msg.optString("playerId");
            this.username = msg.optString("username");
            System.out.printf("[WS] Join riuscita - Player ID: %s, Username: %s%n",
                this.playerId, this.username);
        });

        // Handler per errori join
        onMessageType("join_error", msg -> {
            System.err.println("[WS] Errore join: " + msg.optString("message"));
        });

        // Handler per nuovo giocatore
        onMessageType("player_joined", msg -> {
            System.out.printf("[WS] Giocatore entrato: %s (ID: %s) - Totale: %d%n",
                msg.optString("username"),
                msg.optString("playerId"),
                msg.optInt("players"));
        });

        // Handler per giocatore uscito
        onMessageType("player_left", msg -> {
            System.out.printf("[WS] Giocatore uscito: %s - Rimasti: %d%n",
                msg.optString("playerId"),
                msg.optInt("players"));
        });

        // Handler per messaggi chat
        onMessageType("chat", msg -> {
            System.out.printf("[CHAT] %s: %s (Next: %s)%n",
                msg.optString("username"),
                msg.optString("text"),
                msg.optString("nextPlayerId"));
        });

        // Handler per stato lobby
        onMessageType("state", msg -> {
            System.out.printf("[WS] Stato lobby %s - Giocatori: %d/%d%n",
                msg.optString("lobbyId"),
                msg.optInt("players"),
                msg.optInt("maxPlayers"));

            JSONArray players = msg.optJSONArray("list");
            if (players != null) {
                System.out.println("Lista giocatori:");
                for (int i = 0; i < players.length(); i++) {
                    JSONObject player = players.getJSONObject(i);
                    System.out.printf("  - %s (ID: %s)%n",
                        player.optString("username"),
                        player.optString("playerId"));
                }
            }
        });

        // Handler per lobby piena
        onMessageType("lobby_full", msg -> {
            System.out.println("[WS] La lobby è piena!");
        });

        // Handler per cambio max players
        onMessageType("max_players", msg -> {
            System.out.printf("[WS] Numero massimo giocatori aggiornato: %d%n",
                msg.optInt("value"));
        });

        // Handler per errori generici
        onMessageType("error", msg -> {
            System.err.println("[WS] Errore: " + msg.optString("message"));
        });
    }

    @Override
    public void onOpen(ServerHandshake handshake) {
        isConnected = true;
        System.out.println("[WS] Connessione aperta - Status: " + handshake.getHttpStatus());

        if (onConnectionOpen != null) {
            executor.submit(() -> onConnectionOpen.accept(lobbyId));
        }
    }

    @Override
    public void onMessage(String message) {
        executor.submit(() -> {
            try {
                JSONObject json = new JSONObject(message);
                String type = json.optString("type", "unknown");

                // Chiama l'handler specifico se registrato
                Consumer<JSONObject> handler = messageHandlers.get(type);
                if (handler != null) {
                    handler.accept(json);
                }

            } catch (Exception e) {
                System.err.println("[WS] Errore parsing messaggio: " + e.getMessage());
                if (onError != null) {
                    onError.accept(e);
                }
            }
        });
    }

    @Override
    public void onClose(int code, String reason, boolean remote) {
        isConnected = false;
        System.out.printf("[WS] Connessione chiusa - Code: %d, Reason: %s, Remote: %b%n",
            code, reason, remote);

        if (onConnectionClose != null) {
            executor.submit(() -> onConnectionClose.accept(reason));
        }
    }

    @Override
    public void onError(Exception ex) {
        System.err.println("[WS] Errore: " + ex.getMessage());
        if (onError != null) {
            executor.submit(() -> onError.accept(ex));
        }
    }

    // ==================== METODI PUBBLICI ====================

    /**
     * Registra un handler per un tipo di messaggio specifico
     */
    public LobbyWebSocketClient onMessageType(String type, Consumer<JSONObject> handler) {
        messageHandlers.put(type, handler);
        return this;
    }

    /**
     * Registra callback per apertura connessione
     */
    public LobbyWebSocketClient onConnect(Consumer<String> callback) {
        this.onConnectionOpen = callback;
        return this;
    }

    /**
     * Registra callback per chiusura connessione
     */
    public LobbyWebSocketClient onDisconnect(Consumer<String> callback) {
        this.onConnectionClose = callback;
        return this;
    }

    /**
     * Registra callback per errori
     */
    public LobbyWebSocketClient onErrorCallback(Consumer<Exception> callback) {
        this.onError = callback;
        return this;
    }

    /**
     * Invia un'azione di join alla lobby
     */
    public CompletableFuture<Void> joinLobby(String playerId, String username, String lobbyId) {
        return CompletableFuture.runAsync(() -> {
            JSONObject msg = new JSONObject();
            msg.put("action", "join");
            msg.put("playerId", playerId);
            msg.put("username", username);
            msg.put("lobbyId", lobbyId);
            send(msg.toString());
        }, executor);
    }

    /**
     * Invia un'azione di join alla lobby corrente
     */
    public CompletableFuture<Void> joinLobby(String playerId, String username) {
        return joinLobby(playerId, username, this.lobbyId != null ? this.lobbyId : "default");
    }

    /**
     * Invia un'azione di leave
     */
    public CompletableFuture<Void> leaveLobby() {
        return CompletableFuture.runAsync(() -> {
            JSONObject msg = new JSONObject();
            msg.put("action", "leave");
            send(msg.toString());
        }, executor);
    }

    /**
     * Invia un messaggio in chat
     */
    public CompletableFuture<Void> sendChatMessage(String text) {
        return CompletableFuture.runAsync(() -> {
            JSONObject msg = new JSONObject();
            msg.put("action", "chat");
            msg.put("text", text);
            send(msg.toString());
        }, executor);
    }

    /**
     * Richiede lo stato corrente della lobby
     */
    public CompletableFuture<Void> requestState() {
        return CompletableFuture.runAsync(() -> {
            JSONObject msg = new JSONObject();
            msg.put("action", "state");
            send(msg.toString());
        }, executor);
    }

    /**
     * Imposta il numero massimo di giocatori
     */
    public CompletableFuture<Void> setMaxPlayers(int max) {
        return CompletableFuture.runAsync(() -> {
            JSONObject msg = new JSONObject();
            msg.put("action", "setmax");
            msg.put("max", max);
            send(msg.toString());
        }, executor);
    }

    /**
     * Invia un messaggio JSON personalizzato
     */
    public CompletableFuture<Void> sendCustomMessage(JSONObject message) {
        return CompletableFuture.runAsync(() -> {
            send(message.toString());
        }, executor);
    }

    /**
     * Verifica se la connessione è attiva
     */
    public boolean isConnected() {
        return isConnected && !isClosed();
    }

    /**
     * Riconnetti automaticamente con retry
     */
    public CompletableFuture<Void> reconnectWithRetry(int maxAttempts, int delaySeconds) {
        CompletableFuture<Void> future = new CompletableFuture<>();

        reconnectAttempt(0, maxAttempts, delaySeconds, future);

        return future;
    }

    private void reconnectAttempt(int attempt, int maxAttempts, int delaySeconds,
                                   CompletableFuture<Void> future) {
        if (attempt >= maxAttempts) {
            future.completeExceptionally(
                new Exception("Impossibile riconnettersi dopo " + maxAttempts + " tentativi"));
            return;
        }

        try {
            System.out.printf("[WS] Tentativo di riconnessione %d/%d...%n",
                attempt + 1, maxAttempts);
            reconnectBlocking();
            future.complete(null);
        } catch (Exception e) {
            System.err.printf("[WS] Riconnessione fallita: %s%n", e.getMessage());
            scheduler.schedule(() ->
                reconnectAttempt(attempt + 1, maxAttempts, delaySeconds, future),
                delaySeconds, TimeUnit.SECONDS);
        }
    }

    /**
     * Chiude la connessione e libera le risorse
     */
    public void shutdown() {
        try {
            closeBlocking();
        } catch (Exception e) {
            System.err.println("[WS] Errore chiusura: " + e.getMessage());
        } finally {
            executor.shutdown();
            scheduler.shutdown();
            try {
                if (!executor.awaitTermination(5, TimeUnit.SECONDS)) {
                    executor.shutdownNow();
                }
                if (!scheduler.awaitTermination(5, TimeUnit.SECONDS)) {
                    scheduler.shutdownNow();
                }
            } catch (InterruptedException e) {
                executor.shutdownNow();
                scheduler.shutdownNow();
                Thread.currentThread().interrupt();
            }
        }
    }

    // Getters
    public String getLobbyId() { return lobbyId; }
    public String getPlayerId() { return playerId; }
    public String getUsername() { return username; }
}