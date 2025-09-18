package detalab;

import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.ResourceBundle;
import java.util.List;
import java.util.Arrays;
import java.net.HttpURLConnection;
import detalab.DTO.Response;
import detalab.DTO.LanguageSet;
import org.json.*;

import javafx.scene.control.Alert.AlertType;
import javafx.fxml.FXML;
import javafx.scene.control.Alert;
import javafx.scene.control.ComboBox;
import javafx.scene.control.PasswordField;
import javafx.scene.control.TextField;

public class RegisterPageController extends GeneralPageController {

    @FXML
    TextField emailField;

    @FXML
    TextField usernameField;

    @FXML
    PasswordField passwordField;

    @FXML
    private ComboBox<String> langBox;

    @FXML
    private void register() throws IOException {

        String email = emailField.getText();
        String username = usernameField.getText();
        String password = passwordField.getText();
        String language =  LanguageSet.getCodeFromName(langBox.getValue());

        try {

            // Corpo della richiesta JSON
            String jsonInput = "{"
                        + "\"username\":\"" + username + "\","
                        + "\"email\":\"" + email + "\","
                        + "\"password\":\"" + password + "\","
                        + "\"lingua\":\"" + language + "\""
                        + "}";
    

            Response response = makeRequest("auth/register", "POST", jsonInput, 201);

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
    
            if (response.getStatus() == 201) {
                showAlert(AlertType.INFORMATION, "Successo", "Registrazione avvenuta con successo.", "Benvenuto in DeltaLab!");
                App.setRoot("login");
            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
            }
        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

    }

    @FXML
    private void switchToLogin() throws IOException {
        App.setRoot("login");
    }

    @FXML
    private void loadLanguages() {

        langBox.getItems().addAll(LanguageSet.getLanguageNames());
        langBox.setValue("Italiano"); // Default value

    }

    @FXML
    public void initialize() {
        //Loads Languages in ComboBox
        loadLanguages();
    }

}