package detalab;

import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.ResourceBundle;
import java.util.List;
import java.util.Arrays;

import javafx.fxml.FXML;
import javafx.scene.control.ComboBox;

public class RegisterPageController {

    private List<String> Languages = Arrays.asList("Italiano", "English", "Français", "Deutsch", "Español", "Português", "Nederlands", "Svenska", "Norsk", "Dansk", "Suomi", "Íslenska", "Čeština", "Slovenčina", "Polski", "Magyar", "Română", "Български", "Русский", "Українська", "Ελληνικά", "Türkçe");

    @FXML
    private ComboBox<String> langBox;

    @FXML
    private void switchToLogin() throws IOException {
        App.setRoot("login");
    }

    @FXML
    private void loadLanguages() {

        langBox.getItems().addAll(Languages);
        langBox.setValue("Italiano"); // Default value

    }

    @FXML
    public void initialize() {
        //Loads Languages in ComboBox
        loadLanguages();
    }

}