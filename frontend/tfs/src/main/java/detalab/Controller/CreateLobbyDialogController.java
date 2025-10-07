package detalab.Controller;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import detalab.DTO.LoggedUser;
import detalab.DTO.Response;
import detalab.DTO.User;
import detalab.DTO.CurrentLobby;
import detalab.DTO.Lobby;
import detalab.DTO.LanguageHelper;
import org.json.*;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.Button;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.ComboBox;
import javafx.scene.control.CheckBox;
import javafx.stage.Stage;

public class CreateLobbyDialogController extends GeneralPageController {

  @FXML
  private ComboBox<String> rotationBox;

  @FXML
  private CheckBox privateCheckBox;

  @FXML
  private Label rotationLabel;

  @FXML
  private Label privateLabel;

  @FXML
  private Button createButton;

  @FXML
  private Button cancelButton;

  @FXML
  public void closeDialog() {
    Stage stage = (Stage) rotationBox.getScene().getWindow();
    stage.close();
  }

  @FXML
  private void createLobby() {

    String rotation = rotationBox.getValue();
    boolean isPrivate = privateCheckBox.isSelected();

    try {

      JSONObject json = new JSONObject();
      json.put("idCreator", LoggedUser.getInstance().getId());
      json.put("isPrivate", isPrivate);
      json.put("rotation", rotation.toLowerCase());

      String jsonInput = json.toString();

      System.out.println("JSON Input: " + jsonInput);

      Response response = makeRequest("lobby", "POST", jsonInput, 201);

      System.out.println("result: " + response.getResult());
      System.out.println("status: " + response.getStatus());
      System.out.println("message: " + response.getMessage());
      System.out.println("content: " + response.getContent() + "\n");

      if (response.getStatus() == 201) {
        showAlert(AlertType.INFORMATION, "Successo", "Lobby creata con successo.", "Ora puoi giocare!");
        setLobbySession(new JSONObject(response.getContent()).getString("id"));
        closeDialog();
      } else {
        showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
      }

    } catch (IOException e) {
      e.printStackTrace();
      showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
    }

  }

  private void setLobbySession(String lobbyID) {

    CurrentLobby.cleanLobby();

    try {
      Response response = makeRequest("lobby/" + lobbyID, "GET", 200);

      System.out.println("result: " + response.getResult());
      System.out.println("status: " + response.getStatus());
      System.out.println("message: " + response.getMessage());
      System.out.println("content: " + response.getContent() + "\n");

      if (response.getStatus() == 200) {
        JSONObject obj = new JSONObject(response.getContent());

        Lobby lobby = new Lobby(obj.getString("id"), obj.getInt("utentiConnessi"), obj.getString("rotation"),
            getUsernameById(obj.getString("creator")), obj.getString("status"));

        ArrayList<User> users = new ArrayList<>();
        users.add(LoggedUser.getInstance());
        CurrentLobby.getInstance(lobby, users, new ArrayList<User>());

      } else {
        showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
      }
    } catch (Exception e) {
      e.printStackTrace();
      showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
    }

  }

  private void translateUI() {

    try {

      String translatedText = LanguageHelper
          .translateToNative("Rotation. Private. Create Lobby, Franwik!. Cancel. Spin clockwise. Spin counterclockwise.");
      List<String> translatedStrings = new ArrayList<>();

      for (String str : translatedText.split("\\.")) {
        translatedStrings.add(str.trim());
      }

      // Translate Labels
      rotationLabel.setText(translatedStrings.get(0));
      privateLabel.setText(translatedStrings.get(1));

      // Translate Buttons
      createButton.setText((translatedStrings.get(2).split("\\s+")[0]));
      cancelButton.setText(translatedStrings.get(3));

      // Translate ComboBox
      rotationBox.getItems().clear();
      rotationBox.getItems().addAll(capitalizeLastWord(translatedStrings.get(4)),
          capitalizeLastWord(translatedStrings.get(5)));
      rotationBox.setValue(capitalizeLastWord(translatedStrings.get(4)));

    } catch (Exception e) {

      // Default transations in case of error

      // Labels
      rotationLabel.setText("Rotation");
      privateLabel.setText("Private");

      // Buttons
      createButton.setText("Create");
      cancelButton.setText("Cancel");

      // ComboBox
      rotationBox.getItems().clear();
      rotationBox.getItems().addAll("Clockwise", "Counterclockwise");
      rotationBox.setValue("Clockwise");

      e.printStackTrace();
      showAlert(AlertType.ERROR, "Error", "An error occurred.", "Unable to translate!");
    }
  }

  private String capitalizeLastWord(String input) {
    String lastWord = input.replaceAll(".*\\s", "");
    if (lastWord.isEmpty())
      return lastWord;
    return lastWord.substring(0, 1).toUpperCase() + lastWord.substring(1);
  }

  @FXML
  public void initialize() {
    translateUI();
  }

}