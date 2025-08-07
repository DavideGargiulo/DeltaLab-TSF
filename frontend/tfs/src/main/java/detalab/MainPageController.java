package detalab;

import java.io.IOException;
import javafx.fxml.FXML;

public class MainPageController {

    @FXML
    private void exit() throws IOException {
        App.setRoot("login");
    }

    @FXML
    private void enterGame() throws IOException {
        App.setRoot("game");
    }
}