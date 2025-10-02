package detalab.Controller;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javafx.fxml.FXML;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.layout.VBox;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import detalab.DTO.CurrentLobby;
import detalab.DTO.LanguageHelper;
import detalab.DTO.LoggedUser;
import detalab.App;

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
    private void exit() throws IOException {
        CurrentLobby.cleanLobby();
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
            inputField.setPromptText("Input text...");
            submitButton.setText("Submit");
            exitButton.setText("Exit");

            e.printStackTrace();
            showAlert(AlertType.ERROR, "Error", "An error occurred.", "Unable to translate!");

        }

    }

    private void setItemsNotVisible() {
        player1.setVisible(false);
        player2.setVisible(false);
        player3.setVisible(false);
        player4.setVisible(false);
        player5.setVisible(false);
        player6.setVisible(false);
        player7.setVisible(false);
        phraseLabel.setVisible(false);
    }

    @FXML
    public void initialize() {

        System.out.println(CurrentLobby.getInstance().getLobbyID());

        translateUI();

        setItemsNotVisible();

    }
}