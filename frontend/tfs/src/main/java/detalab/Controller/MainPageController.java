package detalab;

import java.util.ResourceBundle;
import java.net.URL;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import detalab.DTO.LoggedUser;
import detalab.DTO.Response;
import detalab.DTO.Lobby;
import org.json.*;

import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.TableView;
import javafx.scene.control.TableColumn;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.control.Alert.AlertType;
import javafx.scene.control.Alert;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.stage.Modality;
import javafx.scene.layout.BorderPane;



public class MainPageController extends GeneralPageController {

    @FXML
    private BorderPane mainPage;

    @FXML
    Label welcomeLabel;

    @FXML
    private TableView<Lobby> lobbyList;

    @FXML
    private TableColumn<Lobby, String> lobbyID;

    @FXML
    private TableColumn<Lobby, Integer> lobbyUsers;

    @FXML
    private TableColumn<Lobby, String> lobbyRotation;

    @FXML
    private TableColumn<Lobby, String> lobbyCreator;

    @FXML
    private TableColumn<Lobby, String> lobbyStatus;

    @FXML
    private void loadLobbies() {

        try {

            Response response = makeRequest("lobby", "GET", 200);

            String content = response.getContent();

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 200) {
                List<Lobby> lobbies = parseLobbies(content);
                lobbyList.getItems().clear();
                lobbyList.getItems().addAll(lobbies);
            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
            }

            
        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

    }

    private List<Lobby> parseLobbies(String content) {
        
        List<Lobby> lobbies = new ArrayList<>();

        JSONArray array = new JSONArray(content);

        for (int i = 0; i < array.length(); i++) {
            JSONObject obj = array.getJSONObject(i);

            String id = obj.getString("id");
            int users = obj.getInt("utentiConnessi");
            String rotation = obj.getString("rotation");
            String status = obj.getString("status");
            String creator = getUsernameById(obj.getString("creator").trim());

            Lobby lobby = new Lobby(id, users, rotation, creator, status);
            lobbies.add(lobby);
        }

        return lobbies;
    }

    private String getUsernameById(String userID) {
        
        String ret = "Sconosciuto";

        try {

            Response response = makeRequest("user/" + userID, "GET", 200);

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 200) {
                String content = response.getContent();
                JSONObject obj = new JSONObject(content);
                ret = obj.getString("nickname").trim();
            }

        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

        return ret;
    }

    @FXML
    private void showNewLobbyDialog() throws IOException {
    // Carica l'FXML della nuova finestra
    FXMLLoader fxmlLoader = new FXMLLoader(App.class.getResource("NewLobbyDialog.fxml"));
    Parent dialogRoot = fxmlLoader.load();

    // Crea una nuova scena
    Scene dialogScene = new Scene(dialogRoot);

    // Crea il nuovo Stage
    Stage dialogStage = new Stage();
    dialogStage.setTitle("Crea nuova Lobby");
    dialogStage.setScene(dialogScene);

    // Imposta la finestra principale come owner e la modalità modale
    // mainPage è il BorderPane iniettato in questo controller
    Stage ownerStage = (Stage) mainPage.getScene().getWindow();
    dialogStage.initOwner(ownerStage);
    dialogStage.initModality(Modality.WINDOW_MODAL);
    // Se vuoi che blocchi tutta l'app, puoi usare Modality.APPLICATION_MODAL

    // (Opzionale) Imposta dimensioni minime o massime
    dialogStage.setResizable(false);

    // Mostra la finestra in modalità bloccante
    dialogStage.showAndWait();

    // --- qui sotto eventuale logica dopo la chiusura del dialog ---
    // Ad esempio, ricaricare la lista lobbies se la creazione ha avuto successo
    loadLobbies();
}


    @FXML
    private void test() {
        System.out.println("Test button clicked");
    }

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
            welcomeLabel.setText("Benvenuto, " + LoggedUser.getInstance().getUsername() + "!");
        } catch (Exception e) {
            e.printStackTrace();
            welcomeLabel.setText("Benvenuto, utente!");
        }
    }

    @FXML
    public void initialize() {

        setWelcomeMessage();

        //Initalize table columns
        lobbyUsers.setCellValueFactory(new PropertyValueFactory<Lobby, Integer>("lobbyUsers"));
        lobbyID.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyID"));
        lobbyRotation.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyRotation"));
        lobbyCreator.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyCreator"));
        lobbyStatus.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyStatus"));

        loadLobbies();
    }
}