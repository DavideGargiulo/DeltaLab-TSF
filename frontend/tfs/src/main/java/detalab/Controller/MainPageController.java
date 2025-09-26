package detalab;

import java.util.ResourceBundle;
import java.net.URL;
import java.io.IOException;
import java.lang.reflect.Array;
import java.util.List;
import java.util.Random;
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
import javafx.scene.control.TableView;
import javafx.scene.control.TextField;
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
    private TextField codeField;

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
    private Button createLobbyButton;

    @FXML
    private Button randomJoinButton;
    
    @FXML
    private Button joinButton;

    @FXML
    private Button exitButton;

    @FXML
    private Button updateButton;

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

    @FXML
    private void showNewLobbyDialog() {
        try {
            FXMLLoader fxmlLoader = new FXMLLoader(App.class.getResource("NewLobbyDialog.fxml"));
            Parent dialogRoot = fxmlLoader.load();
            Scene dialogScene = new Scene(dialogRoot);
            Stage dialogStage = new Stage();
            dialogStage.setTitle("Crea nuova Lobby");
            dialogStage.setScene(dialogScene);
            Stage ownerStage = (Stage) mainPage.getScene().getWindow();
            dialogStage.initOwner(ownerStage);
            dialogStage.initModality(Modality.WINDOW_MODAL);
            dialogStage.setResizable(false);
            dialogStage.showAndWait();

            // Things to do after the dialog is closed
            loadLobbies();
            enterLobby(CurrentLobby.getInstance());

        } catch (IOException e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Non è stato possibile aprire la finestra.");
        }
    }


    @FXML
    private void exit() throws IOException {
        LoggedUser.cleanUserSession();
        App.setRoot("login");
    }

    @FXML
    private void joinRandomLobby() {    
        if (lobbyList.getItems().isEmpty()) {
            showAlert(AlertType.INFORMATION, "Nessuna lobby disponibile", null, "Non ci sono lobby disponibili a cui unirsi.");
            return;
        }

        ArrayList<Lobby> randomLobby = new ArrayList<>();
        randomLobby.addAll(lobbyList.getItems());

        Random rand = new Random();

        enterLobby(randomLobby.get(rand.nextInt(randomLobby.size())));
    }

    @FXML
    private void joinLobbyWithID() {
        String lobbyID = codeField.getText();
        if (lobbyID.isEmpty()) {
            showAlert(AlertType.INFORMATION, "Codice mancante", null, "Inserisci un codice per unirti a una lobby.");
            return;
        }

        try {
            
            Response response = makeRequest("lobby/" + lobbyID, "GET", 200);

            String content = response.getContent();

            System.out.println("result: " + response.getResult());
            System.out.println("status: " + response.getStatus());
            System.out.println("message: " + response.getMessage());
            System.out.println("content: " + response.getContent() + "\n");

            if (response.getStatus() == 200) {
                JSONObject obj = new JSONObject(content);

                String id = obj.getString("id");
                int users = obj.getInt("utentiConnessi");
                String rotation = obj.getString("rotation");
                String status = obj.getString("status");
                String creator = getUsernameById(obj.getString("creator").trim());

                Lobby lobby = new Lobby(id, users, rotation, creator, status);
                enterLobby(lobby);
            } else {
                showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", response.getMessage());
            }

        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }

        // richiedi lobby con ID = lobbyID
    }

    private void enterLobby(Lobby lobby){

        try {
            CurrentLobby.getInstance(lobby, new ArrayList<User>(), new ArrayList<User>());
            App.setRoot("game");
        } catch (Exception e) {
            e.printStackTrace();
            showAlert(AlertType.ERROR, "Errore", "Si è verificato un errore.", "Errore imprevisto!");
        }
        
        //TODO: Rimuovere il commento quando il backend supporterà l'entrata in lobby

    }

    private void translateUI() {

        try {

            // Translate welcome message
            welcomeLabel.setText(LanguageHelper.translate("Benvenuto") + ",\n" + LoggedUser.getInstance().getUsername() + "!");

            // Translate buttons
            createLobbyButton.setText(LanguageHelper.translate("Create Lobby, Franwik!").split(",")[0].trim());
            randomJoinButton.setText(LanguageHelper.translate("Quick Match"));
            joinButton.setText(LanguageHelper.translate("Join Lobby"));
            exitButton.setText(LanguageHelper.translate("Logout"));
            updateButton.setText(LanguageHelper.translate("Update the table, Franwik!").split("\\s+")[0]);

            // Translate table columns
            lobbyID.setText(LanguageHelper.translate("Code"));
            lobbyUsers.setText(LanguageHelper.translate("Connected Users"));
            lobbyRotation.setText(LanguageHelper.translate("Rotation"));
            lobbyCreator.setText(LanguageHelper.translate("Creator"));
            lobbyStatus.setText(LanguageHelper.translate("Status"));

            // Translate Field
            codeField.setPromptText(LanguageHelper.translate("Lobby Code"));

        } catch (Exception e) {

            // Default translation in case of error

            // Welcome message
            welcomeLabel.setText("Welcome,\nuser!");

            // Buttons
            createLobbyButton.setText("Create Lobby");
            randomJoinButton.setText("Quick Match");
            joinButton.setText("Join");
            exitButton.setText("Log out");
            updateButton.setText("Update");

            // Table columns
            lobbyID.setText("Code");
            lobbyUsers.setText("Connected Users");
            lobbyRotation.setText("Rotation");
            lobbyCreator.setText("Creator");
            lobbyStatus.setText("Status");

            e.printStackTrace();
            showAlert(AlertType.ERROR, "Error", "An error occurred.", "Unable to translate!");
        }
    }

    @FXML
    public void initialize() {

        //Initalize table columns
        lobbyUsers.setCellValueFactory(new PropertyValueFactory<Lobby, Integer>("lobbyUsers"));
        lobbyID.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyID"));
        lobbyRotation.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyRotation"));
        lobbyCreator.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyCreator"));
        lobbyStatus.setCellValueFactory(new PropertyValueFactory<Lobby, String>("lobbyStatus"));

        // Set row factory for lobbyList
        lobbyList.setRowFactory(tv -> {
            javafx.scene.control.TableRow<Lobby> row = new javafx.scene.control.TableRow<>();
            row.setOnMouseClicked(event -> {
                if (event.getClickCount() == 2 && (!row.isEmpty())) {
                    Lobby lobby = row.getItem();
                    System.out.println(lobby.toString());
                    enterLobby(lobby);
                }
            });
            return row;
        });

        loadLobbies();

        translateUI();
    }
}