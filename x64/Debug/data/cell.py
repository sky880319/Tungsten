import os
import os.path as osp
import sys
import torch
import torch.utils.data as data
import torch.nn.functional as F
import cv2
import numpy as np
from .config import cfg
from pycocotools import mask as maskUtils
import random
 
import yaml
from PIL import Image
 
 
# def get_label_map():
#     label_map = {"cell":1}
#     return label_map
 
def get_label_map():
    if cfg.dataset.label_map is None:
        return {x+1: x+1 for x in range(len(cfg.dataset.class_names))}
    else:
        return cfg.dataset.label_map
 
class CellDetection(data.Dataset):
    """`MS Coco Detection <http://mscoco.org/dataset/#detections-challenge2016>`_ Dataset.
    Args:
        root (string): Root directory where images are downloaded to.
        set_name (string): Name of the specific set of COCO images.
        transform (callable, optional): A function/transform that augments the
                                        raw images`
        target_transform (callable, optional): A function/transform that takes
        in the target (bbox) and transforms it.
        prep_crowds (bool): Whether or not to prepare crowds for the evaluation step.
    """
 
    def __init__(self, image_path, mask_path, yaml_path, transform=None,
                 dataset_name='Cell'):
        self.image_path = image_path
        self.ids = os.listdir(self.image_path)
        self.mask_path = mask_path
        self.yaml_path = yaml_path
 
        self.label_map = get_label_map()
 
        self.transform = transform
 
        self.name = dataset_name
 
    def __getitem__(self, index):
        """
        Args:
            index (int): Index
        Returns:
            tuple: Tuple (image, (target, masks, num_crowds)).
                   target is the object returned by ``coco.loadAnns``.
        """
        im, gt, masks, h, w, num_crowds = self.pull_item(index)
        return im, (gt, masks, num_crowds)
 
    def __len__(self):
        return len(self.ids)
 
    def get_obj_index(self, mask):
        n = np.max(mask)
        return n
 
    '''
    標籤形式為
    label_names:
    - gland1
    - gland2
    - gland3
    '''
    def from_yaml_get_class(self, yaml_path):
        with open(yaml_path) as f:
            temp = yaml.load(f.read(), Loader=yaml.FullLoader)
            labels = temp['label_names']
            del labels[0]
        return labels
 
    def get_bbox_from_mask(self, mask):
        coordis = np.where(mask) # 行列，所以coordis[0]表示y，coordis[1]表示x
        height, width = mask.shape[0], mask.shape[1]
        x_min = np.min(coordis[1]) * 1. / width
        x_max = np.max(coordis[1]) * 1. / width
        y_min = np.min(coordis[0]) * 1. / height
        y_max = np.max(coordis[0]) * 1. / height
        return [x_min, y_min, x_max, y_max]
 
    # 重新写draw_mask
    def get_mask_bbox(self, num_obj, mask, image):
        height, width = mask.shape[1], mask.shape[2]
        bbox = []
        for index in range(num_obj):
            for i in range(width):
                for j in range(height):
                    at_pixel = image.getpixel((i, j))
                    # at_pixel = image[j, i]
                    if at_pixel == index + 1:
                        mask[index, j, i] = 1
            bbox.append(self.get_bbox_from_mask(mask[index,:,:]))
        return mask, np.array(bbox)
 
    def pull_item(self, index):
        """
        Args:
            index (int): Index
        Returns:
            tuple: Tuple (image, target, masks, height, width, crowd).
                   target is the object returned by ``coco.loadAnns``.
            Note that if no crowd annotations exist, crowd will be None
        """
        image_name = self.ids[index]
        single_image_path = os.path.join(self.image_path, image_name)
        img = cv2.imread(single_image_path)
        height, width, _ = img.shape
 
        single_mask_path = os.path.join(self.mask_path, image_name.replace(".png", ".npy"))
        mask = np.load(single_mask_path)
        num_obj = self.get_obj_index(mask)
 
        single_yaml_path = os.path.join(self.yaml_path, image_name.replace(".png", ".yaml"))
        labels = self.from_yaml_get_class(single_yaml_path)
 
        assert num_obj == len(labels)
 
        mask_temple = np.zeros([num_obj, height, width], dtype=np.uint8)
        mask, bbox = self.get_mask_bbox(num_obj, mask_temple, mask)
 
        # # 這部分是為了防止標籤重疊，所以就有了，
        # # 前一层删除与当前层及之后所有层重叠的地方
        # occlusion = np.logical_not(mask[-1, :, :]).astype(np.uint8)
        # for i in range(num_obj - 2, -1, -1):
        #     mask[i, :, :] = mask[i, :, :] * occlusion
        #     occlusion = np.logical_and(occlusion, np.logical_not(mask[i, :, :]))
 
        labels_form = []
        for i in range(len(labels)):
            if labels[i].find("cell") != -1:
                labels_form.append("cell")
 
        target = np.zeros((len(labels_form), 5))
        for i in range(len(labels_form)):
            target[i, :4] = bbox[i]
            target[i, 4] = self.label_map[labels_form[i]]
 
        img, mask, boxes, labels = self.transform(img, mask, target[:, :4],
                                                   {'num_crowds': 0, 'labels': target[:, 4]})
 
        num_crowds = labels['num_crowds']
        labels = labels['labels']
 
        target = np.hstack((boxes, np.expand_dims(labels, axis=1)))
 
        return torch.from_numpy(img).permute(2, 0, 1), target, mask, height, width, num_crowds
 
    def pull_image(self, index):
        pass
 
    def pull_anno(self, index):
        pass
 
    def __repr__(self):
        fmt_str = 'Dataset ' + self.__class__.__name__ + '\n'
        fmt_str += '    Number of datapoints: {}\n'.format(self.__len__())
        fmt_str += '    Root Location: {}\n'.format(self.image_path)
        tmp = '    Transforms (if any): '
        fmt_str += '{0}{1}\n'.format(tmp, self.transform.__repr__().replace('\n', '\n' + ' ' * len(tmp)))
        return fmt_str
