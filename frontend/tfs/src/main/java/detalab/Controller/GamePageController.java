package detalab;

import java.io.IOException;
import javafx.fxml.FXML;

public class GamePageController {

    @FXML
    private void exit() throws IOException {
        App.setRoot("main");
    }
}