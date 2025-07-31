package detalab;

import java.io.IOException;
import javafx.fxml.FXML;

public class LoginPageController {

    @FXML
    private void switchToRegister() throws IOException {
        App.setRoot("register");
    }

    @FXML
    private void Login() throws IOException {
        // TODO: Implement login logic here
        // TODO: For now, just switch to the main view
        App.setRoot("main");
    }

}
