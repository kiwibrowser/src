// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Legacy supervised user creation flow screen.
 */

login.createScreen(
    'SupervisedUserCreationScreen', 'supervised-user-creation', function() {
      var MAX_NAME_LENGTH = 50;
      var UserImagesGrid = options.UserImagesGrid;
      var ButtonImages = UserImagesGrid.ButtonImages;

      var ManagerPod = cr.ui.define(function() {
        var node =
            $('supervised-user-creation-manager-template').cloneNode(true);
        node.removeAttribute('id');
        node.removeAttribute('hidden');
        return node;
      });

      ManagerPod.userImageSalt_ = {};

      /**
       * UI element for displaying single account in list of possible managers
       * for new supervised user.
       * @type {Object}
       */
      ManagerPod.prototype = {
        __proto__: HTMLDivElement.prototype,

        /** @override */
        decorate: function() {
          // Mousedown has to be used instead of click to be able to prevent
          // 'focus' event later.
          this.addEventListener('mousedown', this.handleMouseDown_.bind(this));
          var screen = $('supervised-user-creation');
          var managerPod = this;
          var managerPodList = screen.managerList_;
          var hideManagerPasswordError = function(element) {
            managerPod.passwordElement.classList.remove('password-error');
            $('bubble').hide();
          };

          screen.configureTextInput(
              this.passwordElement,
              screen.updateNextButtonForManager_.bind(screen),
              screen.validIfNotEmpty_.bind(screen), function(element) {
                screen.getScreenButton('next').focus();
              }, hideManagerPasswordError);

          this.passwordElement.addEventListener('keydown', function(e) {
            switch (e.key) {
              case 'ArrowUp':
                managerPodList.selectNextPod(-1);
                e.stopPropagation();
                break;
              case 'ArrowDown':
                managerPodList.selectNextPod(+1);
                e.stopPropagation();
                break;
            }
          });
        },

        /**
         * Updates UI elements from user data.
         */
        update: function() {
          this.imageElement.src = 'chrome://userimage/' + this.user.username +
              '?id=' + ManagerPod.userImageSalt_[this.user.username];

          this.nameElement.textContent = this.user.displayName;
          this.emailElement.textContent = this.user.emailAddress;
        },

        showPasswordError: function() {
          this.passwordElement.classList.add('password-error');
          $('bubble').showTextForElement(
              this.passwordElement,
              loadTimeData.getString(
                  'createSupervisedUserWrongManagerPasswordText'),
              cr.ui.Bubble.Attachment.BOTTOM, 24, 4);
        },

        /**
         * Brings focus to password field.
         */
        focusInput: function() {
          this.passwordElement.focus();
        },

        /**
         * Gets image element.
         * @type {!HTMLImageElement}
         */
        get imageElement() {
          return this.querySelector('.supervised-user-creation-manager-image');
        },

        /**
         * Gets name element.
         * @type {!HTMLDivElement}
         */
        get nameElement() {
          return this.querySelector('.supervised-user-creation-manager-name');
        },

        /**
         * Gets e-mail element.
         * @type {!HTMLDivElement}
         */
        get emailElement() {
          return this.querySelector('.supervised-user-creation-manager-email');
        },

        /**
         * Gets password element.
         * @type {!HTMLDivElement}
         */
        get passwordElement() {
          return this.querySelector(
              '.supervised-user-creation-manager-password');
        },

        /**
         * Gets password enclosing block.
         * @type {!HTMLDivElement}
         */
        get passwordBlock() {
          return this.querySelector('.password-block');
        },

        /** @override */
        handleMouseDown_: function(e) {
          this.parentNode.selectPod(this);
          // Prevent default so that we don't trigger 'focus' event.
          e.preventDefault();
        },

        /**
         * The user that this pod represents.
         * @type {!Object}
         */
        user_: undefined,
        get user() {
          return this.user_;
        },
        set user(userDict) {
          this.user_ = userDict;
          this.update();
        },
      };

      var ManagerPodList = cr.ui.define('div');

      /**
       * UI element for selecting manager account for new supervised user.
       * @type {Object}
       */
      ManagerPodList.prototype = {
        __proto__: HTMLDivElement.prototype,

        selectedPod_: null,

        /** @override */
        decorate: function() {},

        /**
         * Returns all the pods in this pod list.
         * @type {NodeList}
         */
        get pods() {
          return this.children;
        },

        addPod: function(manager) {
          var managerPod = new ManagerPod({user: manager});
          this.appendChild(managerPod);
          managerPod.update();
        },

        clearPods: function() {
          this.innerHTML = '';
          this.selectedPod_ = null;
        },

        selectPod: function(podToSelect) {
          if ((this.selectedPod_ == podToSelect) && !!podToSelect) {
            podToSelect.focusInput();
            return;
          }
          this.selectedPod_ = podToSelect;
          for (var i = 0, pod; pod = this.pods[i]; ++i) {
            if (pod != podToSelect) {
              pod.classList.remove('focused');
              pod.passwordElement.value = '';
              pod.passwordBlock.hidden = true;
            }
          }
          if (!podToSelect)
            return;
          podToSelect.classList.add('focused');
          podToSelect.passwordBlock.hidden = false;
          podToSelect.passwordElement.value = '';
          podToSelect.focusInput();
          chrome.send(
              'managerSelectedOnSupervisedUserCreationFlow',
              [podToSelect.user.username]);
        },

        /**
         * Select pod next to currently selected one in given |direction|.
         * @param {integer} direction - +1 for selecting pod below current, -1 for
         *     selecting pod above current.
         * @type {boolean} returns if selected pod has changed.
         */
        selectNextPod: function(direction) {
          if (!this.selectedPod_)
            return false;
          var index = -1;
          for (var i = 0, pod; pod = this.pods[i]; ++i) {
            if (pod == this.selectedPod_) {
              index = i;
              break;
            }
          }
          if (-1 == index)
            return false;
          index = index + direction;
          if (index < 0 || index >= this.pods.length)
            return false;
          this.selectPod(this.pods[index]);
          return true;
        }
      };

      var ImportPod = cr.ui.define(function() {
        var node =
            $('supervised-user-creation-import-template').cloneNode(true);
        node.removeAttribute('id');
        node.removeAttribute('hidden');
        return node;
      });

      /**
       * UI element for displaying single supervised user in list of possible
       * users for importing existing users.
       * @type {Object}
       */
      ImportPod.prototype = {
        __proto__: HTMLDivElement.prototype,

        /** @override */
        decorate: function() {
          // Mousedown has to be used instead of click to be able to prevent
          // 'focus' event later.
          this.addEventListener('mousedown', this.handleMouseDown_.bind(this));
          var screen = $('supervised-user-creation');
          var importList = screen.importList_;
        },

        /**
         * Updates UI elements from user data.
         */
        update: function() {
          this.imageElement.src = this.user.avatarurl;
          this.nameElement.textContent = this.user.name;
          if (this.user.exists) {
            if (this.user.conflict == 'imported') {
              this.nameElement.textContent =
                  loadTimeData.getStringF('importUserExists', this.user.name);
            } else {
              this.nameElement.textContent = loadTimeData.getStringF(
                  'importUsernameExists', this.user.name);
            }
          }
          this.classList.toggle('imported', this.user.exists);
        },

        /**
         * Gets image element.
         * @type {!HTMLImageElement}
         */
        get imageElement() {
          return this.querySelector('.import-pod-image');
        },

        /**
         * Gets name element.
         * @type {!HTMLDivElement}
         */
        get nameElement() {
          return this.querySelector('.import-pod-name');
        },

        /** @override */
        handleMouseDown_: function(e) {
          this.parentNode.selectPod(this);
          // Prevent default so that we don't trigger 'focus' event.
          e.preventDefault();
        },

        /**
         * The user that this pod represents.
         * @type {Object}
         */
        user_: undefined,

        get user() {
          return this.user_;
        },

        set user(userDict) {
          this.user_ = userDict;
          this.update();
        },
      };

      var ImportPodList = cr.ui.define('div');

      /**
       * UI element for selecting existing supervised user for import.
       * @type {Object}
       */
      ImportPodList.prototype = {
        __proto__: HTMLDivElement.prototype,

        selectedPod_: null,

        /** @override */
        decorate: function() {
          this.setAttribute('tabIndex', 0);
          this.classList.add('nofocus');
          var importList = this;
          var screen = $('supervised-user-creation');

          this.addEventListener('focus', function(e) {
            if (importList.selectedPod_ == null) {
              if (importList.pods.length > 0)
                importList.selectPod(importList.pods[0]);
            }
          });

          this.addEventListener('keydown', function(e) {
            switch (e.key) {
              case 'ArrowUp':
                importList.selectNextPod(-1);
                e.stopPropagation();
                break;
              case 'Enter':
                if (importList.selectedPod_ != null)
                  screen.importSupervisedUser_();
                e.stopPropagation();
                break;
              case 'ArrowDown':
                importList.selectNextPod(+1);
                e.stopPropagation();
                break;
            }
          });
        },

        /**
         * Returns all the pods in this pod list.
         * @type {NodeList}
         */
        get pods() {
          return this.children;
        },

        /**
         * Returns selected pod.
         * @type {Node}
         */
        get selectedPod() {
          return this.selectedPod_;
        },

        addPod: function(user) {
          var importPod = new ImportPod({user: user});
          this.appendChild(importPod);
          importPod.update();
        },

        clearPods: function() {
          this.innerHTML = '';
          this.selectedPod_ = null;
        },

        scrollIntoView: function(pod) {
          scroller = this.parentNode;
          var itemHeight = pod.getBoundingClientRect().height;
          var scrollTop = scroller.scrollTop;
          var top = pod.offsetTop - scroller.offsetTop;
          var clientHeight = scroller.clientHeight;

          var self = scroller;

          // Function to adjust the tops of viewport and row.
          function scrollToAdjustTop() {
            self.scrollTop = top;
            return true;
          }
          // Function to adjust the bottoms of viewport and row.
          function scrollToAdjustBottom() {
            var cs = getComputedStyle(self);
            var paddingY =
                parseInt(cs.paddingTop, 10) + parseInt(cs.paddingBottom, 10);

            if (top + itemHeight > scrollTop + clientHeight - paddingY) {
              self.scrollTop = top + itemHeight - clientHeight + paddingY;
              return true;
            }
            return false;
          }

          // Check if the entire of given indexed row can be shown in the
          // viewport.
          if (itemHeight <= clientHeight) {
            if (top < scrollTop)
              return scrollToAdjustTop();
            if (scrollTop + clientHeight < top + itemHeight)
              return scrollToAdjustBottom();
          } else {
            if (scrollTop < top)
              return scrollToAdjustTop();
            if (top + itemHeight < scrollTop + clientHeight)
              return scrollToAdjustBottom();
          }
          return false;
        },

        /**
         * @param {Element} podToSelect - pod to select, can be null.
         */
        selectPod: function(podToSelect) {
          if ((this.selectedPod_ == podToSelect) && !!podToSelect) {
            return;
          }
          this.selectedPod_ = podToSelect;
          for (var i = 0; i < this.pods.length; i++) {
            var pod = this.pods[i];
            if (pod != podToSelect)
              pod.classList.remove('focused');
          }
          if (!podToSelect)
            return;
          podToSelect.classList.add('focused');
          podToSelect.focus();
          var screen = $('supervised-user-creation');
          if (!this.selectedPod_) {
            screen.getScreenButton('import').disabled = true;
          } else {
            screen.getScreenButton('import').disabled =
                this.selectedPod_.user.exists;
            if (!this.selectedPod_.user.exists) {
              chrome.send(
                  'userSelectedForImportInSupervisedUserCreationFlow',
                  [podToSelect.user.id]);
            }
          }
        },

        selectNextPod: function(direction) {
          if (!this.selectedPod_)
            return false;
          var index = -1;
          for (var i = 0, pod; pod = this.pods[i]; ++i) {
            if (pod == this.selectedPod_) {
              index = i;
              break;
            }
          }
          if (-1 == index)
            return false;
          index = index + direction;
          if (index < 0 || index >= this.pods.length)
            return false;
          this.selectPod(this.pods[index]);
          return true;
        },

        selectUser: function(user_id) {
          for (var i = 0, pod; pod = this.pods[i]; ++i) {
            if (pod.user.id == user_id) {
              this.selectPod(pod);
              this.scrollIntoView(pod);
              break;
            }
          }
        },
      };

      return {
        EXTERNAL_API: [
          'loadManagers',
          'setCameraPresent',
          'setDefaultImages',
          'setExistingSupervisedUsers',
          'showErrorPage',
          'showIntroPage',
          'showManagerPage',
          'showManagerPasswordError',
          'showPage',
          'showPasswordError',
          'showProgress',
          'showStatusError',
          'showTutorialPage',
          'showUsernamePage',
          'supervisedUserNameError',
          'supervisedUserNameOk',
          'supervisedUserSuggestImport',
        ],

        lastVerifiedName_: null,
        lastIncorrectUserName_: null,
        managerList_: null,
        importList_: null,

        currentPage_: null,
        imagesRequested_: false,

        // Contains data that can be auto-shared with handler.
        context_: {},

        /** @override */
        decorate: function() {
          this.managerList_ = new ManagerPodList();
          $('supervised-user-creation-managers-pane')
              .appendChild(this.managerList_);

          this.importList_ = new ImportPodList();
          $('supervised-user-creation-import-pane')
              .appendChild(this.importList_);

          var userNameField = $('supervised-user-creation-name');
          var passwordField = $('supervised-user-creation-password');
          var password2Field = $('supervised-user-creation-password-confirm');

          var creationScreen = this;

          var hideUserPasswordError = function(element) {
            $('bubble').hide();
            $('supervised-user-creation-password')
                .classList.remove('password-error');
          };

          this.configureTextInput(
              userNameField, this.checkUserName_.bind(this),
              this.validIfNotEmpty_.bind(this), function(element) {
                passwordField.focus();
              }, this.clearUserNameError_.bind(this));
          this.configureTextInput(
              passwordField, this.updateNextButtonForUser_.bind(this),
              this.validIfNotEmpty_.bind(this), function(element) {
                password2Field.focus();
              }, hideUserPasswordError);
          this.configureTextInput(
              password2Field, this.updateNextButtonForUser_.bind(this),
              this.validIfNotEmpty_.bind(this), function(element) {
                creationScreen.getScreenButton('next').focus();
              }, hideUserPasswordError);

          this.getScreenButton('error').addEventListener('click', function(e) {
            creationScreen.handleErrorButtonPressed_();
            e.stopPropagation();
          });

          /*
          TODO(antrim) : this is an explicit code duplications with
          UserImageScreen. It should be removed by issue 251179.
          */
          var imageGrid = this.getScreenElement('image-grid');
          UserImagesGrid.decorate(imageGrid);

          // Preview image will track the selected item's URL.
          var previewElement = this.getScreenElement('image-preview');
          previewElement.oncontextmenu = function(e) {
            e.preventDefault();
          };

          imageGrid.previewElement = previewElement;
          imageGrid.selectionType = 'default';

          imageGrid.addEventListener(
              'activate', this.handleActivate_.bind(this));
          imageGrid.addEventListener('select', this.handleSelect_.bind(this));
          imageGrid.addEventListener(
              'phototaken', this.handlePhotoTaken_.bind(this));
          imageGrid.addEventListener(
              'photoupdated', this.handlePhotoUpdated_.bind(this));
          // Set the title for camera item in the grid.
          imageGrid.setCameraTitles(
              loadTimeData.getString('takePhoto'),
              loadTimeData.getString('photoFromCamera'));

          this.getScreenElement('take-photo')
              .addEventListener('click', this.handleTakePhoto_.bind(this));
          this.getScreenElement('discard-photo')
              .addEventListener('click', this.handleDiscardPhoto_.bind(this));

          // Toggle 'animation' class for the duration of WebKit transition.
          this.getScreenElement('image-stream-crop')
              .addEventListener('transitionend', function(e) {
                previewElement.classList.remove('animation');
              });
          this.getScreenElement('image-preview-img')
              .addEventListener('transitionend', function(e) {
                previewElement.classList.remove('animation');
              });

          $('supervised-user-creation-navigation')
              .addEventListener('close', this.cancel.bind(this));
        },

        buttonIds: [],

        /**
         * Creates button for adding to controls.
         * @param {string} buttonId -- id for button, have to be unique within
         *   screen. Actual id will be prefixed with screen name and appended
         * with
         *   '-button'. Use getScreenButton(buttonId) to find it later.
         * @param {string} i18nPrefix -- screen prefix for i18n values.
         * @param {function} callback -- will be called on button press with
         *   buttonId parameter.
         * @param {array} pages -- list of pages where this button should be
         *   displayed.
         * @param {array} classes -- list of additional CSS classes for button.
         */
        makeButton: function(buttonId, i18nPrefix, callback, pages, classes) {
          var capitalizedId =
              buttonId.charAt(0).toUpperCase() + buttonId.slice(1);
          this.buttonIds.push(buttonId);
          var result = this.ownerDocument.createElement('button');
          result.id = this.name() + '-' + buttonId + '-button';
          result.classList.add('screen-control-button');
          for (var i = 0; i < classes.length; i++) {
            result.classList.add(classes[i]);
          }
          result.textContent = loadTimeData.getString(
              i18nPrefix + capitalizedId + 'ButtonTitle');
          result.addEventListener('click', function(e) {
            callback(buttonId);
            e.stopPropagation();
          });
          result.pages = pages;
          return result;
        },

        /**
         * Simple validator for |configureTextInput|.
         * Element is considered valid if it has any text.
         * @param {Element} element - element to be validated.
         * @return {boolean} - true, if element has any text.
         */
        validIfNotEmpty_: function(element) {
          return (element.value.length > 0);
        },

        /**
         * Configure text-input |element|.
         * @param {Element} element - element to be configured.
         * @param {function(element)} inputChangeListener - function that will be
         *    called upon any button press/release.
         * @param {function(element)} validator - function that will be called when
         *    Enter is pressed. If it returns |true| then advance to next
         * element.
         * @param {function(element)} moveFocus - function that will determine next
         *    element and move focus to it.
         * @param {function(element)} errorHider - function that is called upon
         *    every button press, so that any associated error can be hidden.
         */
        configureTextInput: function(
            element, inputChangeListener, validator, moveFocus, errorHider) {
          element.addEventListener('keydown', function(e) {
            if (e.key == 'Enter') {
              var dataValid = true;
              if (validator)
                dataValid = validator(element);
              if (!dataValid) {
                element.focus();
              } else {
                if (moveFocus)
                  moveFocus(element);
              }
              e.stopPropagation();
              return;
            }
            if (errorHider)
              errorHider(element);
            if (inputChangeListener)
              inputChangeListener(element);
          });
          element.addEventListener('keyup', function(e) {
            if (inputChangeListener)
              inputChangeListener(element);
          });
        },

        /**
         * Makes element from template.
         * @param {string} templateId -- template will be looked up within screen
         * by class with name "template-<templateId>".
         * @param {string} elementId -- id for result, uinque within screen. Actual
         *   id will be prefixed with screen name. Use getScreenElement(id) to
         * find it later.
         */
        makeFromTemplate: function(templateId, elementId) {
          var templateClassName = 'template-' + templateId;
          var templateNode = this.querySelector('.' + templateClassName);
          var screenPrefix = this.name() + '-';
          var result = templateNode.cloneNode(true);
          result.classList.remove(templateClassName);
          result.id = screenPrefix + elementId;
          return result;
        },

        /**
         * @param {string} buttonId -- id of button to be found,
         * @return {Element} button created by makeButton with given buttonId.
         */
        getScreenButton: function(buttonId) {
          var fullId = this.name() + '-' + buttonId + '-button';
          return this.getScreenElement(buttonId + '-button');
        },

        /**
         * @param {string} elementId -- id of element to be found,
         * @return {Element} button created by makeFromTemplate with elementId.
         */
        getScreenElement: function(elementId) {
          var fullId = this.name() + '-' + elementId;
          return $(fullId);
        },

        /**
         * Screen controls.
         * @type {!Array} Array of Buttons.
         */
        get buttons() {
          var links = this.ownerDocument.createElement('div');
          var buttons = this.ownerDocument.createElement('div');
          links.classList.add('controls-links');
          buttons.classList.add('controls-buttons');

          var importLink = this.makeFromTemplate(
              'import-supervised-user-link', 'import-link');
          importLink.hidden = true;
          links.appendChild(importLink);

          var linkElement = importLink.querySelector('.signin-link');
          linkElement.addEventListener(
              'click', this.importLinkPressed_.bind(this));

          var createLink = this.makeFromTemplate(
              'create-supervised-user-link', 'create-link');
          createLink.hidden = true;
          links.appendChild(createLink);

          var status = this.makeFromTemplate('status-container', 'status');
          buttons.appendChild(status);

          linkElement = createLink.querySelector('.signin-link');
          linkElement.addEventListener(
              'click', this.createLinkPressed_.bind(this));

          buttons.appendChild(this.makeButton(
              'start', 'supervisedUserCreationFlow',
              this.startButtonPressed_.bind(this), ['intro'],
              ['custom-appearance', 'button-fancy', 'button-blue']));

          buttons.appendChild(this.makeButton(
              'prev', 'supervisedUserCreationFlow',
              this.prevButtonPressed_.bind(this), ['manager'], []));

          buttons.appendChild(this.makeButton(
              'next', 'supervisedUserCreationFlow',
              this.nextButtonPressed_.bind(this), ['manager', 'username'], []));

          buttons.appendChild(this.makeButton(
              'import', 'supervisedUserCreationFlow',
              this.importButtonPressed_.bind(this),
              ['import', 'import-password'], []));

          buttons.appendChild(this.makeButton(
              'gotit', 'supervisedUserCreationFlow',
              this.gotItButtonPressed_.bind(this), ['created'],
              ['custom-appearance', 'button-fancy', 'button-blue']));
          return [links, buttons];
        },

        /**
         * Does sanity check and calls backend with current user name/password
         * pair to authenticate manager. May result in showManagerPasswordError.
         * @private
         */
        validateAndLogInAsManager_: function() {
          var selectedPod = this.managerList_.selectedPod_;
          if (null == selectedPod)
            return;

          var managerId = selectedPod.user.username;
          var managerDisplayId = selectedPod.user.emailAddress;
          var managerPassword = selectedPod.passwordElement.value;
          if (managerPassword.length == 0)
            return;
          if (this.disabled)
            return;
          this.disabled = true;
          this.context_.managerId = managerId;
          this.context_.managerDisplayId = managerDisplayId;
          this.context_.managerName = selectedPod.user.displayName;
          chrome.send(
              'authenticateManagerInSupervisedUserCreationFlow',
              [managerId, managerPassword]);
        },

        /**
         * Does sanity check and calls backend with user display name/password
         * pair to create a user.
         * @private
         */
        validateAndCreateSupervisedUser_: function() {
          var firstPassword = $('supervised-user-creation-password').value;
          var secondPassword =
              $('supervised-user-creation-password-confirm').value;
          var userName = $('supervised-user-creation-name').value;
          if (firstPassword != secondPassword) {
            this.showPasswordError(loadTimeData.getString(
                'createSupervisedUserPasswordMismatchError'));
            return;
          }
          if (this.disabled)
            return;
          this.disabled = true;

          this.context_.supervisedName = userName;
          chrome.send(
              'specifySupervisedUserCreationFlowUserData',
              [userName, firstPassword]);
        },

        /**
         * Does sanity check and calls backend with selected existing supervised
         * user id to import user.
         * @private
         */
        importSupervisedUser_: function() {
          if (this.disabled)
            return;
          if (this.currentPage_ == 'import-password') {
            var firstPassword = this.getScreenElement('password').value;
            var secondPassword =
                this.getScreenElement('password-confirm').value;
            if (firstPassword != secondPassword) {
              this.showPasswordError(loadTimeData.getString(
                  'createSupervisedUserPasswordMismatchError'));
              return;
            }
            var userId = this.context_.importUserId;
            this.disabled = true;
            chrome.send(
                'importSupervisedUserWithPassword', [userId, firstPassword]);
            return;
          } else {
            var selectedPod = this.importList_.selectedPod_;
            if (!selectedPod)
              return;
            var user = selectedPod.user;
            var userId = user.id;

            this.context_.importUserId = userId;
            this.context_.supervisedName = user.name;
            this.context_.selectedImageUrl = user.avatarurl;
            if (!user.needPassword) {
              this.disabled = true;
              chrome.send('importSupervisedUser', [userId]);
            } else {
              this.setVisiblePage_('import-password');
            }
          }
        },

        /**
         * Calls backend part to check if current user name is valid/not taken.
         * Results in a call to either supervisedUserNameOk or
         * supervisedUserNameError.
         * @private
         */
        checkUserName_: function() {
          var userName = this.getScreenElement('name').value;

          // Avoid flickering
          if (userName == this.lastIncorrectUserName_ ||
              userName == this.lastVerifiedName_) {
            return;
          }
          if (userName.length > 0) {
            chrome.send('checkSupervisedUserName', [userName]);
          } else {
            this.nameErrorVisible = false;
            this.lastVerifiedName_ = null;
            this.lastIncorrectUserName_ = null;
            this.updateNextButtonForUser_();
          }
        },

        /**
         * Called by backend part in case of successful name validation.
         * @param {string} name - name that was validated.
         */
        supervisedUserNameOk: function(name) {
          this.lastVerifiedName_ = name;
          this.lastIncorrectUserName_ = null;
          if ($('supervised-user-creation-name').value == name)
            this.clearUserNameError_();
          this.updateNextButtonForUser_();
        },

        /**
         * Called by backend part in case of name validation failure.
         * @param {string} name - name that was validated.
         * @param {string} errorText - reason why this name is invalid.
         */
        supervisedUserNameError: function(name, errorText) {
          this.disabled = false;
          this.lastIncorrectUserName_ = name;
          this.lastVerifiedName_ = null;

          var userNameField = $('supervised-user-creation-name');
          if (userNameField.value == this.lastIncorrectUserName_) {
            this.nameErrorVisible = true;
            $('bubble').showTextForElement(
                $('supervised-user-creation-name'), errorText,
                cr.ui.Bubble.Attachment.RIGHT, 12, 4);
            this.setButtonDisabledStatus('next', true);
          }
        },

        supervisedUserSuggestImport: function(name, user_id) {
          this.disabled = false;
          this.lastIncorrectUserName_ = name;
          this.lastVerifiedName_ = null;

          var userNameField = $('supervised-user-creation-name');
          var creationScreen = this;

          if (userNameField.value == this.lastIncorrectUserName_) {
            this.nameErrorVisible = true;
            var link = this.ownerDocument.createElement('div');
            link.innerHTML = loadTimeData.getStringF(
                'importBubbleText', '<a class="signin-link" href="#">', name,
                '</a>');
            link.querySelector('.signin-link')
                .addEventListener('click', function(e) {
                  creationScreen.handleSuggestImport_(user_id);
                  e.stopPropagation();
                });
            $('bubble').showContentForElement(
                $('supervised-user-creation-name'),
                cr.ui.Bubble.Attachment.RIGHT, link, 12, 4);
            this.setButtonDisabledStatus('next', true);
          }
        },

        /**
         * Clears user name error, if name is no more guaranteed to be invalid.
         * @private
         */
        clearUserNameError_: function() {
          // Avoid flickering
          if ($('supervised-user-creation-name').value ==
              this.lastIncorrectUserName_) {
            return;
          }
          this.nameErrorVisible = false;
        },

        /**
         * Called by backend part in case of password validation failure.
         * @param {string} errorText - reason why this password is invalid.
         */
        showPasswordError: function(errorText) {
          $('bubble').showTextForElement(
              $('supervised-user-creation-password'), errorText,
              cr.ui.Bubble.Attachment.RIGHT, 12, 4);
          $('supervised-user-creation-password')
              .classList.add('password-error');
          $('supervised-user-creation-password').focus();
          this.disabled = false;
          this.setButtonDisabledStatus('next', true);
        },

        /**
         * True if user name error should be displayed.
         * @type {boolean}
         */
        set nameErrorVisible(value) {
          $('supervised-user-creation-name')
              .classList.toggle('duplicate-name', value);
          if (!value)
            $('bubble').hide();
        },

        /**
         * Updates state of Continue button after minimal checks.
         * @return {boolean} true, if form seems to be valid.
         * @private
         */
        updateNextButtonForManager_: function() {
          var selectedPod = this.managerList_.selectedPod_;
          canProceed = null != selectedPod &&
              selectedPod.passwordElement.value.length > 0;

          this.setButtonDisabledStatus('next', !canProceed);
          return canProceed;
        },

        /**
         * Updates state of Continue button after minimal checks.
         * @return {boolean} true, if form seems to be valid.
         * @private
         */
        updateNextButtonForUser_: function() {
          var firstPassword = this.getScreenElement('password').value;
          var secondPassword = this.getScreenElement('password-confirm').value;
          var userName = this.getScreenElement('name').value;

          var passwordOk = (firstPassword.length > 0) &&
              (firstPassword.length == secondPassword.length);

          if (this.currentPage_ == 'import-password') {
            this.setButtonDisabledStatus('import', !passwordOk);
            return passwordOk;
          }
          var imageGrid = this.getScreenElement('image-grid');
          var imageChosen =
              !(imageGrid.selectionType == 'camera' && imageGrid.cameraLive);
          var canProceed = passwordOk && (userName.length > 0) &&
              this.lastVerifiedName_ && (userName == this.lastVerifiedName_) &&
              imageChosen;

          this.setButtonDisabledStatus('next', !canProceed);
          return canProceed;
        },

        showSelectedManagerPasswordError_: function() {
          var selectedPod = this.managerList_.selectedPod_;
          selectedPod.showPasswordError();
          selectedPod.passwordElement.value = '';
          selectedPod.focusInput();
          this.updateNextButtonForManager_();
        },

        /**
         * Enables one particular subpage and hides the rest.
         * @param {string} visiblePage - name of subpage.
         * @private
         */
        setVisiblePage_: function(visiblePage) {
          this.disabled = false;
          this.updateText_();
          $('bubble').hide();
          if (!this.imagesRequested_) {
            chrome.send('supervisedUserGetImages');
            this.imagesRequested_ = true;
          }
          var pageNames =
              ['intro', 'manager', 'username', 'import', 'created', 'error'];
          var pageButtons = {
            'intro': 'start',
            'error': 'error',
            'import': 'import',
            'import-password': 'import',
            'created': 'gotit'
          };
          this.hideStatus_();
          var pageToDisplay = visiblePage;
          if (visiblePage == 'import-password')
            pageToDisplay = 'username';

          for (i in pageNames) {
            var pageName = pageNames[i];
            var page = $('supervised-user-creation-' + pageName);
            page.hidden = (pageName != pageToDisplay);
            if (pageName == pageToDisplay)
              $('step-logo').hidden = page.classList.contains('step-no-logo');
          }

          for (i in this.buttonIds) {
            var button = this.getScreenButton(this.buttonIds[i]);
            button.hidden = button.pages.indexOf(visiblePage) < 0;
            button.disabled = false;
          }

          this.getScreenElement('import-link').hidden = true;
          this.getScreenElement('create-link').hidden = true;

          if (pageButtons[visiblePage])
            this.getScreenButton(pageButtons[visiblePage]).focus();

          this.currentPage_ = visiblePage;

          if (visiblePage == 'manager' || visiblePage == 'intro') {
            $('supervised-user-creation-password')
                .classList.remove('password-error');
            if (this.managerList_.pods.length > 0)
              this.managerList_.selectPod(this.managerList_.pods[0]);
            $('login-header-bar').updateUI_();
          }

          if (visiblePage == 'username' || visiblePage == 'import-password') {
            var elements = this.getScreenElement(pageToDisplay)
                               .querySelectorAll('.hide-on-import');
            for (var i = 0; i < elements.length; i++) {
              elements[i].classList.toggle(
                  'hidden-on-import', visiblePage == 'import-password');
            }
          }
          if (visiblePage == 'username') {
            var imageGrid = this.getScreenElement('image-grid');
            // select some image.
            var selected = this.imagesData_[Math.floor(
                Math.random() * this.imagesData_.length)];
            this.context_.selectedImageUrl = selected.url;
            imageGrid.selectedItemUrl = selected.url;
            chrome.send('supervisedUserSelectImage', ['default', selected.url]);
            this.getScreenElement('image-grid').redraw();
            this.checkUserName_();
            this.updateNextButtonForUser_();
            this.getScreenElement('name').focus();
            this.getScreenElement('import-link').hidden =
                this.importList_.pods.length == 0;
          } else if (visiblePage == 'import-password') {
            var imageGrid = this.getScreenElement('image-grid');
            var selected;
            if ('selectedImageUrl' in this.context_) {
              selected = this.context_.selectedImageUrl;
            } else {
              // select some image.
              selected =
                  this.imagesData_[Math.floor(
                                       Math.random() * this.imagesData_.length)]
                      .url;
              chrome.send('supervisedUserSelectImage', ['default', selected]);
            }
            imageGrid.selectedItemUrl = selected;
            this.getScreenElement('image-grid').redraw();

            this.updateNextButtonForUser_();

            this.getScreenElement('password').focus();
            this.getScreenElement('import-link').hidden = true;
          } else {
            this.getScreenElement('image-grid').stopCamera();
          }
          if (visiblePage == 'import') {
            this.getScreenElement('create-link').hidden = false;
            this.getScreenButton('import').disabled =
                !this.importList_.selectedPod_ ||
                this.importList_.selectedPod_.user.exists;
          }
          $('supervised-user-creation-navigation').closeVisible =
              (visiblePage != 'created');

          chrome.send('currentSupervisedUserPage', [this.currentPage_]);
        },

        setButtonDisabledStatus: function(buttonName, status) {
          var button = $('supervised-user-creation-' + buttonName + '-button');
          button.disabled = status;
        },

        gotItButtonPressed_: function() {
          chrome.send('finishLocalSupervisedUserCreation');
        },

        handleErrorButtonPressed_: function() {
          chrome.send('abortLocalSupervisedUserCreation');
        },

        startButtonPressed_: function() {
          this.setVisiblePage_('manager');
          this.setButtonDisabledStatus('next', true);
        },

        nextButtonPressed_: function() {
          if (this.currentPage_ == 'manager') {
            this.validateAndLogInAsManager_();
            return;
          }
          if (this.currentPage_ == 'username') {
            this.validateAndCreateSupervisedUser_();
          }
        },

        importButtonPressed_: function() {
          this.importSupervisedUser_();
        },

        importLinkPressed_: function() {
          this.setVisiblePage_('import');
        },

        handleSuggestImport_: function(user_id) {
          this.setVisiblePage_('import');
          this.importList_.selectUser(user_id);
        },

        createLinkPressed_: function() {
          this.setVisiblePage_('username');
          this.lastIncorrectUserName_ = null;
          this.lastVerifiedName_ = null;
          this.checkUserName_();
        },

        prevButtonPressed_: function() {
          this.setVisiblePage_('intro');
        },

        showProgress: function(text) {
          var status = this.getScreenElement('status');
          var statusText = status.querySelector('.id-text');
          statusText.textContent = text;
          statusText.classList.remove('error');
          status.querySelector('.id-spinner').hidden = false;
          status.hidden = false;
          this.getScreenElement('import-link').hidden = true;
          this.getScreenElement('create-link').hidden = true;
        },

        showStatusError: function(text) {
          var status = this.getScreenElement('status');
          var statusText = status.querySelector('.id-text');
          statusText.textContent = text;
          statusText.classList.add('error');
          status.querySelector('.id-spinner').hidden = true;
          status.hidden = false;
          this.getScreenElement('import-link').hidden = true;
          this.getScreenElement('create-link').hidden = true;
        },

        hideStatus_: function() {
          var status = this.getScreenElement('status');
          status.hidden = true;
        },

        /**
         * Updates state of login header so that necessary buttons are
         * displayed.
         */
        onBeforeShow: function(data) {
          $('login-header-bar').signinUIState =
              SIGNIN_UI_STATE.SUPERVISED_USER_CREATION_FLOW;
          if (data['managers']) {
            this.loadManagers(data['managers']);
          }
          var imageGrid = this.getScreenElement('image-grid');
          imageGrid.updateAndFocus();
        },

        /**
         * Update state of login header so that necessary buttons are displayed.
         */
        onBeforeHide: function() {
          $('login-header-bar').signinUIState = SIGNIN_UI_STATE.HIDDEN;
          this.getScreenElement('image-grid').stopCamera();
        },

        /**
         * Returns a control which should receive an initial focus.
         */
        get defaultControl() {
          return $('supervised-user-creation-name');
        },

        /**
         * True if the the screen is disabled (handles no user interaction).
         * @type {boolean}
         */
        disabled_: false,

        get disabled() {
          return this.disabled_;
        },

        set disabled(value) {
          this.disabled_ = value;
          var controls = this.querySelectorAll('button,input');
          for (var i = 0, control; control = controls[i]; ++i) {
            control.disabled = value;
          }
          $('login-header-bar').disabled = value;
        },

        /**
         * Called by backend part to propagate list of possible managers.
         * @param {Array} userList - list of users that can be managers.
         */
        loadManagers: function(userList) {
          $('supervised-user-creation-managers-block').hidden = false;
          this.managerList_.clearPods();
          for (var i = 0; i < userList.length; ++i)
            this.managerList_.addPod(userList[i]);
          if (userList.length > 0)
            this.managerList_.selectPod(this.managerList_.pods[0]);
        },

        /**
         * Cancels user creation and drops to user screen (either sign).
         */
        cancel: function() {
          var notSignedInPages = ['intro', 'manager'];
          var postCreationPages = ['created'];
          if (notSignedInPages.indexOf(this.currentPage_) >= 0) {
            chrome.send('hideLocalSupervisedUserCreation');

            // Make sure no manager password is kept:
            this.managerList_.clearPods();

            Oobe.showUserPods();
            return;
          }
          if (postCreationPages.indexOf(this.currentPage_) >= 0) {
            chrome.send('finishLocalSupervisedUserCreation');
            return;
          }
          chrome.send('abortLocalSupervisedUserCreation');
        },

        updateText_: function() {
          var managerDisplayId = this.context_.managerDisplayId;
          this.updateElementText_(
              'intro-alternate-text', 'createSupervisedUserIntroAlternateText');
          this.updateElementText_(
              'created-text-1', 'createSupervisedUserCreatedText1',
              this.context_.supervisedName);
          // TODO(antrim): Move wrapping with strong in grd file, and eliminate
          // this
          // call.
          this.updateElementText_(
              'created-text-2', 'createSupervisedUserCreatedText2',
              this.wrapStrong(loadTimeData.getString('managementURL')),
              this.context_.supervisedName);
          this.updateElementText_(
              'created-text-3', 'createSupervisedUserCreatedText3',
              managerDisplayId);
          this.updateElementText_(
              'name-explanation', 'createSupervisedUserNameExplanation',
              managerDisplayId);
        },

        wrapStrong: function(original) {
          if (original == undefined)
            return original;
          return '<strong>' + original + '</strong>';
        },

        updateElementText_: function(localId, templateName) {
          var args = Array.prototype.slice.call(arguments);
          args.shift();
          this.getScreenElement(localId).innerHTML =
              loadTimeData.getStringF.apply(loadTimeData, args);
        },

        showIntroPage: function() {
          $('supervised-user-creation-password').value = '';
          $('supervised-user-creation-password-confirm').value = '';
          $('supervised-user-creation-name').value = '';

          this.lastVerifiedName_ = null;
          this.lastIncorrectUserName_ = null;
          this.passwordErrorVisible = false;
          $('supervised-user-creation-password')
              .classList.remove('password-error');
          this.nameErrorVisible = false;

          this.setVisiblePage_('intro');
        },

        showManagerPage: function() {
          this.setVisiblePage_('manager');
        },

        showUsernamePage: function() {
          this.setVisiblePage_('username');
        },

        showTutorialPage: function() {
          this.setVisiblePage_('created');
        },

        showPage: function(page) {
          this.setVisiblePage_(page);
        },

        showErrorPage: function(errorTitle, errorText, errorButtonText) {
          this.disabled = false;
          $('supervised-user-creation-error-title').innerHTML = errorTitle;
          $('supervised-user-creation-error-text').innerHTML = errorText;
          $('supervised-user-creation-error-button').textContent =
              errorButtonText;
          this.setVisiblePage_('error');
        },

        showManagerPasswordError: function() {
          this.disabled = false;
          this.showSelectedManagerPasswordError_();
        },

        /*
        TODO(antrim) : this is an explicit code duplications with
        UserImageScreen. It should be removed by issue 251179.
        */
        /**
         * Currently selected user image index (take photo button is with zero
         * index).
         * @type {number}
         */
        selectedUserImage_: -1,
        imagesData: [],

        setDefaultImages: function(info) {
          var imageGrid = this.getScreenElement('image-grid');
          // Limit default images to 23 first images of current set to avoid
          // the need to handle overflow and the additional logic that is
          // required to handle that correctly.
          this.imagesData_ = info.images.slice(info.first, info.first + 23);
          imageGrid.setDefaultImages(this.imagesData_);
        },


        handleActivate_: function() {
          var imageGrid = this.getScreenElement('image-grid');
          if (imageGrid.selectedItemUrl == ButtonImages.TAKE_PHOTO) {
            this.handleTakePhoto_();
            return;
          }
          this.nextButtonPressed_();
        },

        /**
         * Handles selection change.
         * @param {Event} e Selection change event.
         * @private
         */
        handleSelect_: function(e) {
          var imageGrid = this.getScreenElement('image-grid');
          this.updateNextButtonForUser_();

          if (imageGrid.cameraLive || imageGrid.selectionType != 'camera')
            imageGrid.previewElement.classList.remove('phototaken');
          else
            imageGrid.previewElement.classList.add('phototaken');

          if (!imageGrid.cameraLive || imageGrid.selectionType != 'camera') {
            this.context_.selectedImageUrl = imageGrid.selectedItemUrl;
            chrome.send(
                'supervisedUserSelectImage',
                [imageGrid.selectionType, imageGrid.selectedItemUrl]);
          }
          // Start/stop camera on (de)selection.
          if (!imageGrid.inProgramSelection &&
              imageGrid.selectionType != e.oldSelectionType) {
            if (imageGrid.selectionType == 'camera') {
              // Programmatic selection of camera item is done in
              // startCamera callback where streaming is started by itself.
              imageGrid.startCamera(function() {
                // Start capture if camera is still the selected item.
                $('supervised-user-creation-image-preview-img')
                    .classList.toggle('animated-transform', true);
                return imageGrid.selectedItem == imageGrid.cameraImage;
              });
            } else {
              $('supervised-user-creation-image-preview-img')
                  .classList.toggle('animated-transform', false);
              imageGrid.stopCamera();
            }
          }
        },

        /**
         * Handle photo capture from the live camera stream.
         */
        handleTakePhoto_: function(e) {
          this.getScreenElement('image-grid').takePhoto();
          chrome.send('supervisedUserTakePhoto');
        },

        handlePhotoTaken_: function(e) {
          chrome.send('supervisedUserPhotoTaken', [e.dataURL]);
          announceAccessibleMessage(
              loadTimeData.getString('photoCaptureAccessibleText'));
        },

        /**
         * Handle photo updated event.
         * @param {Event} e Event with 'dataURL' property containing a data URL.
         */
        handlePhotoUpdated_: function(e) {
          chrome.send('supervisedUserPhotoTaken', [e.dataURL]);
        },

        /**
         * Handle discarding the captured photo.
         */
        handleDiscardPhoto_: function(e) {
          var imageGrid = this.getScreenElement('image-grid');
          imageGrid.discardPhoto();
          chrome.send('supervisedUserDiscardPhoto');
          announceAccessibleMessage(
              loadTimeData.getString('photoDiscardAccessibleText'));
        },

        setCameraPresent: function(present) {
          this.getScreenElement('image-grid').cameraPresent = present;
        },

        setExistingSupervisedUsers: function(users) {
          var selectedUser = null;
          // Store selected user
          if (this.importList_.selectedPod)
            selectedUser = this.importList_.selectedPod.user.id;

          var userList = users;
          userList.sort(function(a, b) {
            // Put existing users last.
            if (a.exists != b.exists)
              return a.exists ? 1 : -1;
            // Sort rest by name.
            return a.name.localeCompare(b.name, [], {sensitivity: 'base'});
          });

          this.importList_.clearPods();
          var selectedIndex = -1;
          for (var i = 0; i < userList.length; ++i) {
            this.importList_.addPod(userList[i]);
            if (selectedUser == userList[i].id)
              selectedIndex = i;
          }

          if (userList.length == 1)
            this.importList_.selectPod(this.importList_.pods[0]);

          if (selectedIndex >= 0)
            this.importList_.selectPod(this.importList_.pods[selectedIndex]);

          if (this.currentPage_ == 'username')
            this.getScreenElement('import-link').hidden =
                (userList.length == 0);
        },
      };
    });
