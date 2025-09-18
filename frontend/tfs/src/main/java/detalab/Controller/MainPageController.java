package detalab;

import java.io.IOException;
import detalab.DTO.LoggedUser;

import javafx.fxml.FXML;
import javafx.scene.control.Label;

public class MainPageController {

    @FXML
    Label welcomeLabel;

    @FXML
    private void exit() throws IOException {
        LoggedUser.cleanUserSession();
        App.setRoot("login");
    }

    @FXML
    private void enterGame() throws IOException {
        App.setRoot("game");
    }

    private void setWelcomeMessage() {
        try {
            welcomeLabel.setText("Benvenuto, " + LoggedUser.getInstance().getEmail() + "!");
        } catch (Exception e) {
            e.printStackTrace();
            welcomeLabel.setText("Benvenuto, utente!");
        }
    }

    @FXML
    public void initialize() {
        setWelcomeMessage();
    }
}