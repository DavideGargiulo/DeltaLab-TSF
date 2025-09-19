package detalab;

import java.io.IOException;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import detalab.DTO.CurrentLobby;
import detalab.DTO.Lobby;

public class GamePageController {

    @FXML
    Label codeLabel;

    @FXML
    private void exit() throws IOException {
        CurrentLobby.cleanLobby();
        App.setRoot("main");
    }

    private void setCodeLabel() {
        codeLabel.setText("Codice:\n" + CurrentLobby.getInstance().getLobbyID());
    }

    @FXML
    public void initialize() {

        setCodeLabel();

    }
}