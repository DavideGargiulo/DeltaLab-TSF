package detalab;

import java.io.IOException;
import javafx.fxml.FXML;

public class RegisterPageController {

    @FXML
    private void switchToLogin() throws IOException {
        App.setRoot("login");
    }
}