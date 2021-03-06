<?php
/**
 * This file contains the NDB_Client class
 *
 * PHP Version 5
 *
 * @category Main
 * @package  Main
 * @author   Sebastien <unknown@example.com>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/Loris/
 */
require_once 'Utility.class.inc';

/**
 * NeuroDB client - takes care of loading the configuration, creating the first
 * database connection, and including a lot of commonly used libraries.
 * By default does web-appropriate setup (session management and user auth).
 *
 * @category Main
 * @package  Main
 * @author   Sebastien <unknown@example.com>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://www.github.com/aces/Loris/
 */
class NDB_Client
{
    /**
    * Defines whether session management and user authentication is performed
    */
    var $_isWebClient = true;

    /**
    * Defines that the client is a command line client
    *
    * @return none
    */
    function makeCommandLine()
    {
        $this->_isWebClient = false;
    }

    /**
    * Initializes the environment
    *
    * @param string $configFile Path on filesystem to config.xml file to use
    *
    * @return true on success
    */
    function initialize($configFile = null)
    {
        // require all relevant OO class libraries
        include_once "NDB_Config.class.inc";
        include_once "Database.class.inc";
        include_once "SinglePointLogin.class.inc";
        include_once "State.class.inc";
        include_once "User.class.inc";
        include_once "Site.class.inc";
        include_once 'NDB_Caller.class.inc';

        include_once "Candidate.class.inc";
        include_once "NDB_BVL_Battery.class.inc";
        include_once "NDB_BVL_Instrument.class.inc";
        include_once "NDB_BVL_InstrumentStatus.class.inc";
        include_once "TimePoint.class.inc";
        include_once "Utility.class.inc";

        include_once "NDB_BVL_Feedback.class.inc";

        // Load the config file.
        $factory = NDB_Factory::singleton();

        $config = $factory->config($configFile);

        // Load the database object with settings from config
        // so that after this we can just call Database::singleton()
        // with no parameters
        $DBSettings = $config->getSetting("database");
        if (!defined("UNIT_TESTING")) {
            $DB = Database::singleton(
                $DBSettings['database'],
                $DBSettings['username'],
                $DBSettings['password'],
                $DBSettings['host'],
                true
            );
        }

        // Now make sure the factory reference is the same as the
        // singleton reference
        $DB = $factory->database();

        // add extra include paths. This must be done
        // after the database is connected, since the setting
        // can come from the database.
        $extLibs = $config->getSetting('extLibs');
        if (!empty($extLibs)) {
            set_include_path($extLibs.":".get_include_path());
        }

        // This must be done after the extra includes, as those may
        // include Smarty
        include_once "Smarty_hook.class.inc";

        if (Utility::isErrorX($DB)) {
            die("Could not connect to database: ".$DB->getMessage());
        }
        $GLOBALS['DB'] =& $DB;

        // tell Pear to print errors if the config says so
        if ($config->getSetting('showPearErrors')) {
            $GLOBALS['_PEAR_default_error_mode'] = PEAR_ERROR_PRINT;
            //$setoptions  = &$GLOBALS['_PEAR_default_error_options'];
        }

        // stop here if this is a command line client
        if (!$this->_isWebClient) {
            return true;
        }

        // start php session
        session_start();

        // if exiting, destroy the php session
        if (isset($_REQUEST['logout']) && $_REQUEST['logout']) {
            session_destroy();
            setcookie("FeedbackButtonBoolean", "");
            session_start();
        }

        /**
        * Log In procedures
        */
        if (!isset($_SESSION['State'])) {
            $_SESSION['State'] =& State::singleton();
            $login = new SinglePointLogin();
            $_SESSION['State']->setProperty('login', $login);
        } else {
            $login = $_SESSION['State']->getProperty('login');
        }
        if (!$login->isLoggedIn()) {
            $auth = $login->authenticate();
            if (Utility::isErrorX($auth)) {
            } elseif ($auth === true) {
                $error = $login->setState();
            } elseif ($auth === false) {
                header("HTTP/1.1 403 Forbidden");
                $login->clearState();
            }
        }
        if (!$login->isLoggedIn()) {
            // display login window
            //ob_end_clean();
            $login->showLoginScreen();
            //terminate script
            die();
        }

        $user =& User::singleton($_SESSION['State']->getUsername());
        if (Utility::isErrorX($user)) {
             die("Could not validate user: ".$user->getMessage());
        }

        // finished initializing
        return true;
    }

    /**
     * Returns true if the user is logged in
     *
     * @return true iff user is logged in
     */
    function isLoggedIn()
    {
        if (!isset($_SESSION['State'])) {
            $_SESSION['State'] =& State::singleton();
            $login = new SinglePointLogin();
            $_SESSION['State']->setProperty('login', $login);
        } else {
            $login = $_SESSION['State']->getProperty('login');
        }

        return $login->isLoggedIn();
    }
}
?>
