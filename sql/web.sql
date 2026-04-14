SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for account
-- ----------------------------
DROP TABLE IF EXISTS `account`;
CREATE TABLE `account`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(50) CHARACTER SET utf8mb3 COLLATE utf8mb3_bin NOT NULL,
  `password` char(32) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL,
  `question` varchar(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `answer` varchar(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `email` varchar(64) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `qq` varchar(16) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `tel` varchar(16) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `address` varchar(255) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `id_type` enum('IdCard') CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT 'IdCard',
  `id_card` varchar(32) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `point` int NULL DEFAULT 0,
  `is_online` smallint UNSIGNED NOT NULL DEFAULT 0,
  `is_lock` smallint UNSIGNED NOT NULL DEFAULT 0,
  `user_name` varchar(50) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  PRIMARY KEY (`id`, `name`) USING BTREE,
  UNIQUE INDEX `id`(`id` ASC) USING BTREE,
  UNIQUE INDEX `name`(`name` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 0 CHARACTER SET = utf8mb3 COLLATE = utf8mb3_general_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for account_banip
-- ----------------------------
DROP TABLE IF EXISTS `account_banip`;
CREATE TABLE `account_banip`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `host` char(60) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL,
  `tips` varchar(255) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL COMMENT '备注',
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8mb3 COLLATE = utf8mb3_general_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for account_cfg
-- ----------------------------
DROP TABLE IF EXISTS `account_cfg`;
CREATE TABLE `account_cfg`  (
  `charguid` int NOT NULL COMMENT '玩家GUID',
  `isgm` int NOT NULL DEFAULT 0 COMMENT '是否拥有GM权限',
  PRIMARY KEY (`charguid`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = latin1 COLLATE = latin1_swedish_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for account_prize
-- ----------------------------
DROP TABLE IF EXISTS `account_prize`;
CREATE TABLE `account_prize`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `account` varchar(50) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL COMMENT '账号',
  `world` int NOT NULL DEFAULT 0 COMMENT '世界ID',
  `charguid` int UNSIGNED NOT NULL DEFAULT 0 COMMENT '玩家GUID',
  `itemid` int UNSIGNED NOT NULL DEFAULT 0 COMMENT '物品ID',
  `itemnum` int NOT NULL COMMENT '物品数量',
  `isget` smallint NOT NULL COMMENT '是否领取了',
  `validtime` int NOT NULL COMMENT '有效期，时间格式为unix时间',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `id`(`id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8mb3 COLLATE = utf8mb3_general_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for server
-- ----------------------------
DROP TABLE IF EXISTS `server`;
CREATE TABLE `server`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(32) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL,
  `host` char(60) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 2 CHARACTER SET = utf8mb3 COLLATE = utf8mb3_general_ci ROW_FORMAT = DYNAMIC;

SET FOREIGN_KEY_CHECKS = 1;
