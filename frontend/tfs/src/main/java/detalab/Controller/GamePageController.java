package detalab.Controller;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import javafx.fxml.FXML;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.layout.VBox;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ListView;
import javafx.scene.control.TextField;
import detalab.DTO.CurrentLobby;
import detalab.DTO.LanguageHelper;
import detalab.DTO.LobbyWebSocketClient;
import detalab.DTO.LoggedUser;
import detalab.DTO.User;
import detalab.App;
import org.json.JSONArray;
import org.json.JSONObject;

public class GamePageController extends GeneralPageController {

    @FXML
    Label codeLabel;

    @FXML
    Label playingLabel;

    @FXML
    Label playingCount;

    @FXML
    Label waitingLabel;

    @FXML
    Button exitButton;

    @FXML
    Button submitButton;

    @FXML
    TextField inputField;

    @FXML
    Label phraseLabel;

    @FXML
    ListView<String> waitingList;

    private class PlayerUI {
        VBox container;
        Label username;

        PlayerUI(VBox container, Label username) {
            this.container = container;
            this.username = username;
        }
    }

    private List<PlayerUI> players;

    @FXML
    VBox player0;

    @FXML
    Label username0;

    @FXML
    VBox player1;

    @FXML
    Label username1;

    @FXML
    VBox player2;

    @FXML
    Label username2;

    @FXML
    VBox player3;

    @FXML
    Label username3;

    @FXML
    VBox player4;

    @FXML
    Label username4;

    @FXML
    VBox player5;

    @FXML
    Label username5;

    @FXML
    VBox player6;

    @FXML
    Label username6;

    @FXML
    VBox player7;

    @FXML
    Label username7;

    @FXML
    private void submit() {
        String inputText = inputField.getText().trim();

        try {
            if (inputText.isEmpty()) {
                throw new IllegalArgumentException("Input text cannot be empty.");
            }
            client.sendChatMessage(inputText).get();
            inputField.clear();
        } catch (Exception e) {
            showAlert(AlertType.ERROR, "Invalid Input", "Please enter a valid text.", e.getMessage());
            inputField.clear();
            return;
        }

    }

    @FXML
    private void exit() throws IOException {
        CurrentLobby.cleanLobby();
        if (client != null) {
            try {
                client.leaveLobby().get();
                client.shutdown();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        App.setRoot("main");
    }

    private String capitalize(String str) {
        if (str == null || str.isEmpty()) {
            return str;
        }
        return str.substring(0, 1).toUpperCase() + str.substring(1);
    }

    private void translateUI() {

        username0.setText(LoggedUser.getInstance().getUsername());

        // Translate UI elements

        try {

            String translatedText = LanguageHelper.translate("In Game;Waiting;Exit;Code;Text;Send");

            List<String> translatedStrings = new ArrayList<>();
            for (String str : translatedText.split("\\;")) {
                translatedStrings.add(str.trim());
            }

            codeLabel.setText(capitalize(translatedStrings.get(3)) + ": " + CurrentLobby.getInstance().getLobbyID());
            playingLabel.setText(capitalize(translatedStrings.get(0)));
            waitingLabel.setText(capitalize(translatedStrings.get(1)));
            inputField.setPromptText(capitalize(translatedStrings.get(4)));
            submitButton.setText(capitalize(translatedStrings.get(5)));
            exitButton.setText(capitalize(translatedStrings.get(2)));

        } catch (Exception e) {

            // Default translation in case of error

            codeLabel.setText("Code: " + CurrentLobby.getInstance().getLobbyID());
            playingLabel.setText("Playing");
            waitingLabel.setText("Waiting");
            inputField.setPromptText("Text");
            submitButton.setText("Send");
            exitButton.setText("Exit");

            e.printStackTrace();
            showAlert(AlertType.ERROR, "Error", "An error occurred.", "Unable to translate!");

        }

    }

    private void setItemsNotVisible() {
        for (PlayerUI player : players) {
            player.container.setVisible(false);
        }
        phraseLabel.setVisible(false);
    }

    CurrentLobby lobby = CurrentLobby.getInstance();
    LoggedUser user = LoggedUser.getInstance();

    LobbyWebSocketClient client;

    public GamePageController() {
        try {
            client = new LobbyWebSocketClient("localhost", 8080, lobby.getLobbyID());
            setupWebSocketHandlers();
        } catch (Exception e) {
            e.printStackTrace();
            client = null;
        }
    }


    // Configura tutti gli handler per i messaggi WebSocket
    private void setupWebSocketHandlers() {

        // TODO: implement all handlers


    }

    //Aggiorna il contatore dei giocatori con formato n/max
    private void updatePlayersCount(int totalPlayers) {
        playingCount.setText(totalPlayers + "/8");
    }

    private void loadPlayers() {
        CurrentLobby lobby = CurrentLobby.getInstance();

        ArrayList<User> activePlayers = lobby.getPlayers();
        ArrayList<User> waitingPlayers = lobby.getSpectators();

        System.out.println("Rotation: " + lobby.getLobbyRotation());

        // System.out.println("\n----------\n");
        // System.out.println("Active Players:");
        // for (User activeUser : activePlayers) {
        //     System.out.println(activeUser.getUsername());
        // }
        // System.out.println("\n----------\n");

        // TODO: fix this workaround
        if (!activePlayers.get(0).getUsername().equals(LoggedUser.getInstance().getUsername())) {
            if (lobby.getLobbyRotation().equals("orario")) {
                // TODO: fix difference between orario and clockwise
                for (int i = activePlayers.size() - 1, uiIndex = players.size() - 1; i >= 0 && uiIndex >= 0; i--, uiIndex--) {
                    User activeUser = activePlayers.get(i);
                    PlayerUI playerUI = players.get(uiIndex);
                    playerUI.container.setVisible(true);
                    playerUI.username.setText(activeUser.getUsername());
                }

            } else {

                for (int i = activePlayers.size() - 1, uiIndex = 0; i >= 0 && uiIndex < players.size(); i--, uiIndex++) {
                    User activeUser = activePlayers.get(i);
                    PlayerUI playerUI = players.get(uiIndex);
                    playerUI.container.setVisible(true);
                    playerUI.username.setText(activeUser.getUsername());
                }

            }
        }

        for (User player : waitingPlayers) {
            waitingList.getItems().add(player.getUsername());
        }

        updatePlayersCount(activePlayers.size());
    }


    @FXML
    public void initialize() {

        translateUI();

        players = Arrays.<PlayerUI>asList(
            new PlayerUI(player1, username1),
            new PlayerUI(player2, username2),
            new PlayerUI(player3, username3),
            new PlayerUI(player4, username4),
            new PlayerUI(player5, username5),
            new PlayerUI(player6, username6),
            new PlayerUI(player7, username7)
        );

        setItemsNotVisible();

        loadPlayers();

        try {
            client.connectBlocking();
            client.joinLobby(String.valueOf(user.getId()), user.getUsername(), lobby.getLobbyID()).get();
        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Connection Error",
                "Unable to connect to the lobby", e.getMessage());
        }
    }
}
