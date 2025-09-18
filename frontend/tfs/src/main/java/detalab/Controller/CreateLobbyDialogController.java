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



public class CreateLobbyDialogController extends GeneralPageController {

    @FXML
    Label welcomeLabel;

    @FXML
    public void initialize() {

    }
}