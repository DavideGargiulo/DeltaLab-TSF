package detalab.Controller;

import java.io.IOException;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import detalab.DTO.CurrentLobby;
import detalab.App;

public class GamePageController {

    @FXML
    Label codeLabel;

    @FXML
    private void exit() throws IOException {
        CurrentLobby.cleanLobby();
        App.setRoot("main");
    }

    private void setCodeLabel() {
        codeLabel.setText("Code: " + CurrentLobby.getInstance().getLobbyID());
    }

    @FXML
    public void initialize() {

        System.out.println(CurrentLobby.getInstance().getLobbyID());

        setCodeLabel();

    }
}